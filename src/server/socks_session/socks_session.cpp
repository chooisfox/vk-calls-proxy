#include "socks_session.hpp"

#include <QHostAddress>
#include <QtEndian>

namespace SERVER
{

SocksSession::SocksSession(QTcpSocket* socket, uint32_t id, QObject* parent) : QObject(parent), m_socket(socket), m_id(id)
{
	m_socket->setParent(this);
	m_socket->setReadBufferSize(512 * 1024);

	connect(m_socket, &QTcpSocket::readyRead, this, &SocksSession::onReadyRead);

	connect(m_socket, &QTcpSocket::disconnected, this, [this] {
		m_isPaused = false;
		while (m_socket->bytesAvailable() > 0)
		{
			QByteArray chunk = m_socket->read(65536);
			if (m_state == State::Tunneling)
			{
				emit dataReceived(m_id, chunk);
			}
		}
		emit sessionClosed(m_id);
	});
}

SocksSession::~SocksSession()
{
	if (m_socket->isOpen())
		m_socket->close();
}

void SocksSession::start()
{}

void SocksSession::pauseReading()
{
	m_isPaused = true;
}

void SocksSession::resumeReading()
{
	m_isPaused = false;
	if (m_socket->bytesAvailable() > 0)
	{
		onReadyRead();
	}
}

void SocksSession::onReadyRead()
{
	if (m_isPaused)
		return;

	constexpr qint64 MAX_CHUNK_SIZE = 65536;
	int				 chunksRead		= 0;

	while (m_socket->bytesAvailable() > 0 && !m_isPaused && chunksRead < 8)
	{
		QByteArray chunk = m_socket->read(MAX_CHUNK_SIZE);
		m_buffer.append(chunk);

		switch (m_state)
		{
			case State::Handshake:
				processHandshake();
				break;
			case State::Request:
				processRequest();
				break;
			case State::Tunneling:
				if (!m_buffer.isEmpty())
				{
					emit dataReceived(m_id, m_buffer);
					m_buffer.clear();
				}
				break;
		}
		chunksRead++;
	}

	if (m_socket->bytesAvailable() > 0 && !m_isPaused)
	{
		QMetaObject::invokeMethod(this, "onReadyRead", Qt::QueuedConnection);
	}
}

void SocksSession::processHandshake()
{
	if (m_buffer.size() < 2)
		return;
	if (m_buffer[0] != 0x05)
	{
		closeSession();
		return;
	}

	const int numMethods = m_buffer[1];
	if (m_buffer.size() < 2 + numMethods)
		return;

	constexpr char reply[] = {0x05, 0x00};
	m_socket->write(reply, sizeof(reply));

	m_buffer.remove(0, 2 + numMethods);
	m_state = State::Request;

	if (!m_buffer.isEmpty())
		processRequest();
}

void SocksSession::processRequest()
{
	if (m_buffer.size() < 4)
		return;
	if (m_buffer[0] != 0x05 || m_buffer[1] != 0x01)
	{
		closeSession();
		return;
	}

	QString	 targetHost;
	uint16_t targetPort = 0;
	int		 headerSize = 0;

	const char atype = m_buffer[3];
	if (atype == 0x01)
	{
		if (m_buffer.size() < 10)
			return;
		targetHost = QHostAddress(qFromBigEndian<uint32_t>(m_buffer.constData() + 4)).toString();
		targetPort = qFromBigEndian<uint16_t>(m_buffer.constData() + 8);
		headerSize = 10;
	}
	else if (atype == 0x03)
	{
		const int domainLen = m_buffer[4];
		if (m_buffer.size() < 5 + domainLen + 2)
			return;
		targetHost = QString::fromUtf8(m_buffer.mid(5, domainLen));
		targetPort = qFromBigEndian<uint16_t>(m_buffer.constData() + 5 + domainLen);
		headerSize = 5 + domainLen + 2;
	}
	else if (atype == 0x04)
	{
		if (m_buffer.size() < 22)
			return;
		Q_IPV6ADDR ipv6;
		std::memcpy(ipv6.c, m_buffer.constData() + 4, 16);
		targetHost = QString("[%1]").arg(QHostAddress(ipv6).toString());
		targetPort = qFromBigEndian<uint16_t>(m_buffer.constData() + 20);
		headerSize = 22;
	}
	else
	{
		closeSession();
		return;
	}

	m_buffer.remove(0, headerSize);
	emit requestRemoteConnect(m_id, QString("%1:%2").arg(targetHost).arg(targetPort));
}

void SocksSession::onConnectOK()
{
	m_state						  = State::Tunneling;
	constexpr char successReply[] = {0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
	m_socket->write(successReply, sizeof(successReply));

	if (!m_buffer.isEmpty())
	{
		emit dataReceived(m_id, m_buffer);
		m_buffer.clear();
	}
}

void SocksSession::onConnectErr(const QString&)
{
	constexpr char failReply[] = {0x05, 0x05, 0x00, 0x01, 0, 0, 0, 0, 0, 0};
	m_socket->write(failReply, sizeof(failReply));
	closeSession();
}

void SocksSession::sendData(const QByteArray& data)
{
	if (m_socket->state() == QAbstractSocket::ConnectedState)
	{
		m_socket->write(data);
	}
}

void SocksSession::closeSession()
{
	m_socket->disconnectFromHost();
}

} // namespace SERVER

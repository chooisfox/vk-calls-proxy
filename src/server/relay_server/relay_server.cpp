#include "relay_server.hpp"

#include "serialization.hpp"
#include "spdlog_wrapper.hpp"

namespace SERVER
{

std::string_view RelayServer::get_manager_name() const
{
	return "RelayServer";
}

void RelayServer::initialize()
{
	m_wsServer	  = new QWebSocketServer("VK Relay", QWebSocketServer::NonSecureMode, this);
	m_socksServer = new QTcpServer(this);
	SPD_INFO_CLASS(COMMON::d_settings_group_server, "RelayServer initialized.");
}

RelayServer::~RelayServer()
{
	stop();
}

bool RelayServer::start(uint16_t wsPort, uint16_t socksPort)
{
	stop();

	if (!m_wsServer->listen(QHostAddress::LocalHost, wsPort))
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_server, fmt::format("Failed WS port {}", wsPort));
		return false;
	}
	if (!m_socksServer->listen(QHostAddress::LocalHost, socksPort))
	{
		SPD_ERROR_CLASS(COMMON::d_settings_group_server, fmt::format("Failed SOCKS port {}", socksPort));
		m_wsServer->close();
		return false;
	}

	connect(m_wsServer, &QWebSocketServer::newConnection, this, &RelayServer::onNewWsConnection, Qt::UniqueConnection);
	connect(m_socksServer, &QTcpServer::newConnection, this, &RelayServer::onNewSocksConnection, Qt::UniqueConnection);

	emit logMessage(QString("Service Relay started. SOCKS5: %1 | WS: %2").arg(socksPort).arg(wsPort));
	return true;
}

void RelayServer::stop()
{
	if (m_wsServer->isListening())
		m_wsServer->close();
	if (m_socksServer->isListening())
		m_socksServer->close();

	if (m_activeWs)
	{
		m_activeWs->close();
		m_activeWs->deleteLater();
		m_activeWs = nullptr;
	}

	qDeleteAll(m_sessions);
	m_sessions.clear();

	m_isWsPaused = false;
}

void RelayServer::onNewWsConnection()
{
	if (m_activeWs)
	{
		m_activeWs->close();
		m_activeWs->deleteLater();
	}
	m_activeWs = m_wsServer->nextPendingConnection();
	m_activeWs->setReadBufferSize(1024 * 1024);

	connect(m_activeWs, &QWebSocket::binaryMessageReceived, this, &RelayServer::onWsBinaryMessage);
	connect(m_activeWs, &QWebSocket::disconnected, this, &RelayServer::onWsDisconnected);

	m_isWsPaused = false;
	emit logMessage("WebRTC tunnel successfully connected to local relay.");
	emit vpnStatusChanged(true);
}

void RelayServer::onWsBinaryMessage(const QByteArray& msg)
{
	const auto frameOpt = API::PROTOCOL::decode(msg);
	if (!frameOpt.has_value())
		return;

	const auto& frame = frameOpt.value();

	using API::PROTOCOL::MsgType;

	if (frame.connId == 0)
	{
		if (frame.type == MsgType::PauseRx)
		{
			m_isWsPaused = true;
			pauseAllSessions();
		}
		else if (frame.type == MsgType::ResumeRx)
		{
			m_isWsPaused = false;
			resumeAllSessions();
		}
		else if (frame.type == MsgType::CallInvite)
		{
			QString link = QString::fromUtf8(frame.payload);
			emit	callInviteReceived(link);
		}
		return;
	}

	if (auto it = m_sessions.find(frame.connId); it != m_sessions.end())
	{
		SocksSession* session = it.value();
		switch (frame.type)
		{
			case MsgType::ConnectOK:
				session->onConnectOK();
				break;
			case MsgType::ConnectErr:
				session->onConnectErr(QString::fromUtf8(frame.payload));
				break;
			case MsgType::Data:
				session->sendData(frame.payload);
				break;
			case MsgType::Close:
				session->closeSession();
				m_sessions.erase(it);
				session->deleteLater();
				break;
			default:
				break;
		}
	}
}

void RelayServer::onNewSocksConnection()
{
	QTcpSocket*	   socket = m_socksServer->nextPendingConnection();
	const uint32_t id	  = m_nextConnId++;

	auto* session = new SocksSession(socket, id, this);
	m_sessions.insert(id, session);

	if (m_isWsPaused)
	{
		session->pauseReading();
	}

	connect(session, &SocksSession::requestRemoteConnect, this, &RelayServer::handleSocksConnect);
	connect(session, &SocksSession::dataReceived, this, &RelayServer::handleSocksData);

	connect(session, &SocksSession::sessionClosed, this, [this, id]() {
		handleSocksClosed(id);
	});

	session->start();
}

void RelayServer::handleSocksConnect(uint32_t id, const QString& hostPort)
{
	sendFrame(id, API::PROTOCOL::MsgType::Connect, hostPort.toUtf8());
}

void RelayServer::handleSocksData(uint32_t id, QByteArray data)
{
	sendFrame(id, API::PROTOCOL::MsgType::Data, std::move(data));
}

void RelayServer::handleSocksClosed(uint32_t id)
{
	sendFrame(id, API::PROTOCOL::MsgType::Close);

	if (m_sessions.contains(id))
	{
		SocksSession* s = m_sessions.take(id);
		s->deleteLater();
	}
}

void RelayServer::pauseAllSessions()
{
	for (SocksSession* session : std::as_const(m_sessions))
	{
		session->pauseReading();
	}
}

void RelayServer::resumeAllSessions()
{
	for (SocksSession* session : std::as_const(m_sessions))
	{
		session->resumeReading();
	}
}

void RelayServer::sendFrame(uint32_t connId, API::PROTOCOL::MsgType type, const QByteArray& payload)
{
	if (!m_activeWs || m_activeWs->state() != QAbstractSocket::ConnectedState)
		return;
	API::PROTOCOL::Frame frame {connId, type, payload};
	m_activeWs->sendBinaryMessage(API::PROTOCOL::encode(frame));
}

void RelayServer::onWsDisconnected()
{
	emit logMessage("WebRTC tunnel aborted.");
	emit vpnStatusChanged(false);

	m_isWsPaused = false;
	resumeAllSessions();

	if (m_activeWs)
	{
		m_activeWs->deleteLater();
		m_activeWs = nullptr;
	}
}

} // namespace SERVER

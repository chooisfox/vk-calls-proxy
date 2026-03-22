#include "creator_server.hpp"

#include "serialization.hpp"

namespace SERVER
{

std::string_view CreatorServer::get_manager_name() const
{
	return "CreatorServer";
}

void CreatorServer::initialize()
{
	m_wsServer = new QWebSocketServer("VK Creator Relay", QWebSocketServer::NonSecureMode, this);
}

CreatorServer::~CreatorServer()
{
	stop();
}

bool CreatorServer::start(uint16_t wsPort)
{
	stop();
	if (!m_wsServer->listen(QHostAddress::Any, wsPort))
	{
		emit logMessage(QString("[ERROR] Failed to start Creator service on port %1.").arg(wsPort));
		return false;
	}

	connect(m_wsServer, &QWebSocketServer::newConnection, this, &CreatorServer::onNewWsConnection, Qt::UniqueConnection);
	emit logMessage(QString("[INFO] Creator service started. Awaiting WebRTC hook on port WS: %1").arg(wsPort));
	return true;
}

void CreatorServer::stop()
{
	if (m_wsServer->isListening())
		m_wsServer->close();

	if (m_activeWs)
	{
		m_activeWs->close();
		m_activeWs->deleteLater();
		m_activeWs = nullptr;
	}

	for (auto& conn : m_connections)
	{
		conn.socket->close();
		conn.socket->deleteLater();
	}
	m_connections.clear();
	m_isWsPaused = false;
}

void CreatorServer::sendCallInviteLink(const QString& link)
{
	sendFrame(0, API::PROTOCOL::MsgType::CallInvite, link.toUtf8());
	emit logMessage(QString("[INFO] Sent Call Invite Link to Client: %1").arg(link));
}

void CreatorServer::onNewWsConnection()
{
	if (m_activeWs)
	{
		m_activeWs->close();
		m_activeWs->deleteLater();
	}
	m_activeWs = m_wsServer->nextPendingConnection();
	m_activeWs->setReadBufferSize(1024 * 1024);

	connect(m_activeWs, &QWebSocket::binaryMessageReceived, this, &CreatorServer::onWsBinaryMessage);
	connect(m_activeWs, &QWebSocket::disconnected, this, &CreatorServer::onWsDisconnected);

	m_isWsPaused = false;
	emit logMessage("[INFO] WebRTC Hook successfully connected to Creator Server.");
}

void CreatorServer::onWsBinaryMessage(const QByteArray& msg)
{
	const auto frameOpt = API::PROTOCOL::decode(msg);
	if (!frameOpt.has_value())
		return;

	const auto&	   frame = frameOpt.value();
	const uint32_t id	 = frame.connId;

	using API::PROTOCOL::MsgType;

	if (id == 0)
	{
		if (frame.type == MsgType::PauseRx)
		{
			m_isWsPaused = true;
		}
		else if (frame.type == MsgType::ResumeRx)
		{
			m_isWsPaused = false;
			resumeAllTargets();
		}
		return;
	}

	switch (frame.type)
	{
		case MsgType::Connect: {
			QString targetAddr = QString::fromUtf8(frame.payload);
			int		lastColon  = targetAddr.lastIndexOf(':');
			if (lastColon == -1)
				return;

			QString	 host = targetAddr.left(lastColon);
			uint16_t port = targetAddr.mid(lastColon + 1).toUShort();

			if (host.startsWith('[') && host.endsWith(']'))
				host = host.mid(1, host.length() - 2);

			auto* socket = new QTcpSocket(this);

			socket->setReadBufferSize(512 * 1024);

			TargetConnection connCtx {socket, host, port, QElapsedTimer()};
			connCtx.latencyTimer.start();
			m_connections.insert(id, connCtx);

			connect(socket, &QTcpSocket::connected, this, [this, id]() {
				onTargetConnected(id);
			});
			connect(socket, &QTcpSocket::errorOccurred, this, [this, id](auto) {
				if (m_connections.contains(id))
					onTargetError(id, m_connections[id].socket->errorString());
			});
			connect(socket, &QTcpSocket::readyRead, this, [this, id]() {
				onTargetReadyRead(id);
			});
			connect(socket, &QTcpSocket::disconnected, this, [this, id]() {
				onTargetDisconnected(id);
			});

			if (m_isDebugMode)
			{
				emit logMessage(QString("[CONNECT] ID: %1 | TARGET: %2:%3 | STATUS: Pending").arg(id).arg(host).arg(port), true);
			}
			socket->connectToHost(host, port);
			break;
		}
		case MsgType::Data: {
			if (m_connections.contains(id))
			{
				auto& conn = m_connections[id];
				conn.socket->write(frame.payload);
				if (m_isDebugMode)
				{
					emit logMessage(QString("[DATA] [TX] WS -> %1:%2 | SIZE: %3 bytes").arg(conn.host).arg(conn.port).arg(frame.payload.size()),
									true);
				}
			}
			break;
		}
		case MsgType::Close: {
			if (m_connections.contains(id))
			{
				QTcpSocket* socket = m_connections.take(id).socket;
				socket->close();
				socket->deleteLater();
				if (m_isDebugMode)
				{
					emit logMessage(QString("[CLOSE] ID: %1 | SOURCE: WS Client").arg(id), true);
				}
			}
			break;
		}
		default:
			break;
	}
}

void CreatorServer::onTargetConnected(uint32_t connId)
{
	if (m_connections.contains(connId))
	{
		auto&  conn	   = m_connections[connId];
		qint64 latency = conn.latencyTimer.elapsed();
		if (m_isDebugMode)
		{
			emit logMessage(
				QString("[CONNECT] ID: %1 | TARGET: %2:%3 | STATUS: OK | LATENCY: %4 ms").arg(connId).arg(conn.host).arg(conn.port).arg(latency),
				true);
		}
	}
	sendFrame(connId, API::PROTOCOL::MsgType::ConnectOK);
}

void CreatorServer::onTargetError(uint32_t connId, const QString& errorString)
{
	if (m_isDebugMode && m_connections.contains(connId))
	{
		auto& conn = m_connections[connId];
		emit  logMessage(QString("[ERROR] ID: %1 | TARGET: %2:%3 | REASON: %4").arg(connId).arg(conn.host).arg(conn.port).arg(errorString), true);
	}
	sendFrame(connId, API::PROTOCOL::MsgType::ConnectErr, errorString.toUtf8());

	if (m_connections.contains(connId))
	{
		QTcpSocket* s = m_connections.take(connId).socket;
		s->deleteLater();
	}
}

void CreatorServer::onTargetReadyRead(uint32_t connId)
{
	if (m_isWsPaused || !m_connections.contains(connId))
		return;

	auto& conn = m_connections[connId];

	constexpr qint64 MAX_CHUNK_SIZE = 65536;
	int				 chunksRead		= 0;

	while (conn.socket->bytesAvailable() > 0 && !m_isWsPaused && chunksRead < 8)
	{
		QByteArray data = conn.socket->read(MAX_CHUNK_SIZE);
		if (m_isDebugMode)
		{
			emit logMessage(QString("[DATA] [RX] %1:%2 -> WS | SIZE: %3 bytes").arg(conn.host).arg(conn.port).arg(data.size()), true);
		}
		sendFrame(connId, API::PROTOCOL::MsgType::Data, data);
		chunksRead++;
	}

	if (conn.socket->bytesAvailable() > 0 && !m_isWsPaused)
	{
		QMetaObject::invokeMethod(
			this,
			[this, connId]() {
				onTargetReadyRead(connId);
			},
			Qt::QueuedConnection);
	}
}

void CreatorServer::resumeAllTargets()
{
	for (auto it = m_connections.begin(); it != m_connections.end(); ++it)
	{
		if (it.value().socket->bytesAvailable() > 0)
		{
			onTargetReadyRead(it.key());
		}
	}
}

void CreatorServer::onTargetDisconnected(uint32_t connId)
{
	if (m_connections.contains(connId))
	{
		auto& conn = m_connections[connId];

		while (conn.socket->bytesAvailable() > 0)
		{
			QByteArray data = conn.socket->read(65536);
			if (m_isDebugMode)
			{
				emit logMessage(QString("[DATA] [FLUSH] %1:%2 -> WS | SIZE: %3 bytes").arg(conn.host).arg(conn.port).arg(data.size()), true);
			}
			sendFrame(connId, API::PROTOCOL::MsgType::Data, data);
		}

		sendFrame(connId, API::PROTOCOL::MsgType::Close);

		if (m_isDebugMode)
		{
			emit logMessage(QString("[CLOSE] ID: %1 | TARGET: %2:%3 | SOURCE: Remote Host").arg(connId).arg(conn.host).arg(conn.port), true);
		}

		QTcpSocket* s = m_connections.take(connId).socket;
		s->deleteLater();
	}
}

void CreatorServer::sendFrame(uint32_t connId, API::PROTOCOL::MsgType type, const QByteArray& payload)
{
	if (!m_activeWs || m_activeWs->state() != QAbstractSocket::ConnectedState)
		return;
	API::PROTOCOL::Frame frame {connId, type, payload};
	m_activeWs->sendBinaryMessage(API::PROTOCOL::encode(frame));
}

void CreatorServer::onWsDisconnected()
{
	emit logMessage("[WARN] Connection with browser hook lost.");
	m_isWsPaused = false;
	if (m_activeWs)
	{
		m_activeWs->deleteLater();
		m_activeWs = nullptr;
	}
}

} // namespace SERVER

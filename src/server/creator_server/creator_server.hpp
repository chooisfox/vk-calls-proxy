#pragma once
#include "enums.hpp"
#include "frame.hpp"
#include "manager_singleton.hpp"

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTcpSocket>
#include <QWebSocket>
#include <QWebSocketServer>

namespace SERVER
{

struct TargetConnection
{
	QTcpSocket*	  socket;
	QString		  host;
	uint16_t	  port;
	QElapsedTimer latencyTimer;
};

class CreatorServer final : public QObject, public UTILS::ManagerSingleton<CreatorServer>
{
	Q_OBJECT
	friend class UTILS::ManagerSingleton<CreatorServer>;

public:
	[[nodiscard]] std::string_view get_manager_name() const override;
	void						   initialize() override;

	[[nodiscard]] bool start(uint16_t wsPort = 9000);
	void			   stop();

	void setDebugMode(bool enabled)
	{
		m_isDebugMode = enabled;
	}

	void sendCallInviteLink(const QString& link);

signals:
	void logMessage(const QString& msg, bool isDebug = false);

private slots:
	void onNewWsConnection();
	void onWsBinaryMessage(const QByteArray& msg);
	void onWsDisconnected();

	void onTargetConnected(uint32_t connId);
	void onTargetError(uint32_t connId, const QString& errorString);
	void onTargetReadyRead(uint32_t connId);
	void onTargetDisconnected(uint32_t connId);

private:
	CreatorServer() = default;
	~CreatorServer() override;

	void sendFrame(uint32_t connId, API::PROTOCOL::MsgType type, const QByteArray& payload = {});
	void resumeAllTargets();

	QWebSocketServer* m_wsServer {nullptr};
	QWebSocket*		  m_activeWs {nullptr};
	bool			  m_isDebugMode {false};

	bool m_isWsPaused {false};

	QHash<uint32_t, TargetConnection> m_connections;
};

} // namespace SERVER

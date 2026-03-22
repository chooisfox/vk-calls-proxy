#pragma once
#include "enums.hpp"
#include "manager_singleton.hpp"
#include "socks_session.hpp"

#include <QHash>
#include <QObject>
#include <QTcpServer>
#include <QWebSocket>
#include <QWebSocketServer>

namespace SERVER
{

class RelayServer final : public QObject, public UTILS::ManagerSingleton<RelayServer>
{
	Q_OBJECT
	friend class UTILS::ManagerSingleton<RelayServer>;

public:
	[[nodiscard]] std::string_view get_manager_name() const override;
	void						   initialize() override;

	[[nodiscard]] bool start(uint16_t wsPort = 9000, uint16_t socksPort = 1080);
	void			   stop();

signals:
	void logMessage(const QString& msg);
	void vpnStatusChanged(bool active);
	void callInviteReceived(const QString& link);

private slots:
	void onNewWsConnection();
	void onWsBinaryMessage(const QByteArray& msg);
	void onWsDisconnected();

	void onNewSocksConnection();
	void handleSocksConnect(uint32_t id, const QString& hostPort);
	void handleSocksData(uint32_t id, QByteArray data);
	void handleSocksClosed(uint32_t id);

private:
	RelayServer() = default;
	~RelayServer() override;

	void sendFrame(uint32_t connId, API::PROTOCOL::MsgType type, const QByteArray& payload = {});

	void pauseAllSessions();
	void resumeAllSessions();

	QWebSocketServer* m_wsServer {nullptr};
	QTcpServer*		  m_socksServer {nullptr};
	QWebSocket*		  m_activeWs {nullptr};

	bool m_isWsPaused {false};

	uint32_t					   m_nextConnId {1};
	QHash<uint32_t, SocksSession*> m_sessions;
};

} // namespace SERVER

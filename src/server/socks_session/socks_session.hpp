#pragma once
#include <QByteArray>
#include <QObject>
#include <QTcpSocket>

namespace SERVER
{

class SocksSession final : public QObject
{
	Q_OBJECT
public:
	explicit SocksSession(QTcpSocket* socket, uint32_t id, QObject* parent = nullptr);
	~SocksSession() override;

	void start();
	void sendData(const QByteArray& data);
	void closeSession();

	void onConnectOK();
	void onConnectErr(const QString& err);

	void pauseReading();
	void resumeReading();

	[[nodiscard]] uint32_t id() const noexcept
	{
		return m_id;
	}

signals:
	void requestRemoteConnect(uint32_t id, const QString& hostPort);
	void dataReceived(uint32_t id, QByteArray data);
	void sessionClosed(uint32_t id);

private slots:
	void onReadyRead();

private:
	enum class State
	{
		Handshake,
		Request,
		Tunneling
	};

	void processHandshake();
	void processRequest();

	QTcpSocket* m_socket;
	uint32_t	m_id;
	State		m_state {State::Handshake};
	QByteArray	m_buffer;

	bool m_isPaused {false};
};

} // namespace SERVER

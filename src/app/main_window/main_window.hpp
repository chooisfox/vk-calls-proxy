#pragma once
#include "browser_panel.hpp"
#include "call_control_panel.hpp"
#include "settings_panel.hpp"

#include <QMainWindow>
#include <QTextEdit>

namespace APP::UI
{

enum class ServiceState
{
	Stopped,
	RunningClient,
	RunningCreator
};

enum class CallAutomationState
{
	Idle,
	CreatorHosting,
	ClientAutoJoining,
	ClientAnonJoining
};

class MainWindow final : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

	void triggerAutoStart();

public slots:
	void appendLog(const QString& msg, const QString& color = "#B0B0B0");

private slots:
	void onToggleServer();
	void onModeChanged(int mode);
	void onPageLoaded(bool ok);

	void onToggleCreatorAutoHost(bool active);
	void onToggleClientAutoJoin(bool active);
	void onTriggerClientAnonJoin();

private:
	void changeServiceState(ServiceState newState);
	void changeCallState(CallAutomationState newState);
	void stopAllAutomation();

	SettingsPanel*	  m_settingsPanel {nullptr};
	CallControlPanel* m_callControlPanel {nullptr};
	BrowserPanel*	  m_browserPanel {nullptr};
	QTextEdit*		  m_logView {nullptr};

	ServiceState		m_serviceState {ServiceState::Stopped};
	CallAutomationState m_callState {CallAutomationState::Idle};
};

} // namespace APP::UI

#pragma once
#include "browser_panel.hpp"
#include "call_control_panel.hpp"
#include "settings_panel.hpp"

#include <QMainWindow>
#include <QTextEdit>

namespace APP::UI
{

class MainWindow final : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

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
	SettingsPanel*	  m_settingsPanel;
	CallControlPanel* m_callControlPanel;
	BrowserPanel*	  m_browserPanel;
	QTextEdit*		  m_logView;

	bool m_isServerRunning {false};
	bool m_isCreatorAutoHostActive {false};
	bool m_isClientAutoJoinActive {false};
	bool m_isClientAnonJoinActive {false};
};

} // namespace APP::UI

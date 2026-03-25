#include "main_window.hpp"

#include "creator_server.hpp"
#include "relay_server.hpp"

#include <QSplitter>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

namespace APP::UI
{

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
	setWindowTitle("VK Call Proxy");
	resize(1280, 800);

	auto* central	 = new QWidget(this);
	auto* mainLayout = new QVBoxLayout(central);

	m_settingsPanel	   = new SettingsPanel();
	m_callControlPanel = new CallControlPanel();
	m_browserPanel	   = new BrowserPanel();

	m_logView = new QTextEdit();
	m_logView->setReadOnly(true);
	m_logView->setStyleSheet(
		"background-color: #1E1E1E; color: #ABB2BF; font-family: Consolas, monospace; font-size: 13px; border: none; padding: 5px;");
	m_logView->setMinimumHeight(100);

	auto* splitter = new QSplitter(Qt::Vertical);
	splitter->addWidget(m_browserPanel);
	splitter->addWidget(m_logView);

	splitter->setStretchFactor(0, 7);
	splitter->setStretchFactor(1, 2);
	splitter->setSizes(QList<int>({800, 200}));

	mainLayout->addWidget(m_settingsPanel);
	mainLayout->addWidget(m_callControlPanel);
	mainLayout->addWidget(splitter);
	setCentralWidget(central);

	connect(m_settingsPanel, &SettingsPanel::modeChanged, this, &MainWindow::onModeChanged);
	connect(m_settingsPanel, &SettingsPanel::toggleServerRequested, this, &MainWindow::onToggleServer);
	connect(m_settingsPanel, &SettingsPanel::debugToggled, [](bool e) {
		SERVER::CreatorServer::instance()->setDebugMode(e);
	});

	connect(m_callControlPanel, &CallControlPanel::toggleCreatorHostRequested, this, &MainWindow::onToggleCreatorAutoHost);
	connect(m_callControlPanel, &CallControlPanel::toggleClientAutoJoinRequested, this, &MainWindow::onToggleClientAutoJoin);
	connect(m_callControlPanel, &CallControlPanel::triggerClientAnonJoinRequested, this, &MainWindow::onTriggerClientAnonJoin);

	connect(m_browserPanel, &BrowserPanel::logRequested, this, [this](const QString& msg, const QString& c) {
		appendLog(msg, c);
	});
	connect(m_browserPanel, &BrowserPanel::pageLoaded, this, &MainWindow::onPageLoaded);
	connect(m_browserPanel, &BrowserPanel::callLinkExtracted, this, [this](const QString& link) {
		appendLog("Call Link Extracted! You can copy it now.", "#6A8759");
		m_callControlPanel->setInviteLink(link);
	});

	connect(SERVER::RelayServer::instance().get(), &SERVER::RelayServer::logMessage, this, [this](const QString& msg) {
		appendLog("[Relay] " + msg, "#6897BB");
	});
	connect(SERVER::CreatorServer::instance().get(), &SERVER::CreatorServer::logMessage, this, [this](const QString& msg, bool isDebug) {
		if (isDebug && !m_settingsPanel->isDebugMode())
			return;
		appendLog("[Creator] " + msg, isDebug ? "#7E9CD8" : "#C678DD");
	});

	m_settingsPanel->loadSettings();
	appendLog("Application Loaded. Choose mode and click Start Service.", "#ABB2BF");
}

MainWindow::~MainWindow()
{
	changeServiceState(ServiceState::Stopped);
}

void MainWindow::triggerAutoStart()
{
	QTimer::singleShot(500, this, [this]() {
		if (m_serviceState == ServiceState::Stopped)
		{
			appendLog("Auto-starting service from command line...", "#D19A66");
			onToggleServer();
		}
	});
}

void MainWindow::onModeChanged(int mode)
{
	if (m_serviceState != ServiceState::Stopped)
	{
		appendLog("Mode changed while service was running. Stopping service...", "#E06C75");
		changeServiceState(ServiceState::Stopped);
	}

	m_callControlPanel->updateVisibilityForMode(mode);
	m_browserPanel->setupProfileForMode(mode);
	appendLog(mode == 0 ? "Switched to Client Mode." : "Switched to Creator Mode.", "#B0B0B0");
}

void MainWindow::onToggleServer()
{
	if (m_serviceState != ServiceState::Stopped)
	{
		changeServiceState(ServiceState::Stopped);
	}
	else
	{
		changeServiceState(m_settingsPanel->getMode() == 0 ? ServiceState::RunningClient : ServiceState::RunningCreator);
	}
}

void MainWindow::changeServiceState(ServiceState newState)
{
	if (m_serviceState == newState)
		return;

	if (m_serviceState != ServiceState::Stopped)
	{
		SERVER::RelayServer::instance()->stop();
		SERVER::CreatorServer::instance()->stop();
		stopAllAutomation();
		m_settingsPanel->setServerRunningState(false);
		appendLog("Service successfully stopped.", "#E06C75");
	}

	m_serviceState = newState;

	if (m_serviceState == ServiceState::RunningClient)
	{
		if (SERVER::RelayServer::instance()->start(m_settingsPanel->getWsPort(), m_settingsPanel->getSocksPort()))
		{
			m_settingsPanel->setServerRunningState(true);
			m_browserPanel->injectHooks(m_settingsPanel->getWsPort());
			appendLog("Client Service started securely. Scripts injected to intercept WebRTC events.", "#98C379");
		}
		else
		{
			appendLog("[ERROR] Failed to start Client service. Check if ports are in use.", "#E06C75");
			m_serviceState = ServiceState::Stopped;
			m_settingsPanel->setServerRunningState(false);
		}
	}
	else if (m_serviceState == ServiceState::RunningCreator)
	{
		if (SERVER::CreatorServer::instance()->start(m_settingsPanel->getWsPort()))
		{
			m_settingsPanel->setServerRunningState(true);
			m_browserPanel->injectHooks(m_settingsPanel->getWsPort());
			appendLog("Creator Service started securely. Scripts injected to intercept WebRTC events.", "#98C379");
		}
		else
		{
			appendLog("[ERROR] Failed to start Creator service. Check if ports are in use.", "#E06C75");
			m_serviceState = ServiceState::Stopped;
			m_settingsPanel->setServerRunningState(false);
		}
	}
}

void MainWindow::stopAllAutomation()
{
	if (m_callState == CallAutomationState::CreatorHosting)
	{
		m_browserPanel->runJavaScript("window.postMessage('STOP_CREATOR', '*');");
		m_callControlPanel->setCreatorHostActive(false);
	}
	else if (m_callState == CallAutomationState::ClientAutoJoining)
	{
		m_browserPanel->runJavaScript("window.postMessage('STOP_CLIENT_USER', '*');");
		m_callControlPanel->setClientAutoJoinActive(false);
	}

	m_callState = CallAutomationState::Idle;
}

void MainWindow::changeCallState(CallAutomationState newState)
{
	if (m_callState == newState)
		return;

	stopAllAutomation();
	m_callState = newState;

	switch (m_callState)
	{
		case CallAutomationState::CreatorHosting: {
			QString user = m_callControlPanel->getTargetUser();
			appendLog("Creator Auto-Host started. Maintaining call and monitoring drops...", "#D19A66");
			m_browserPanel->runJavaScript(QString("window.postMessage('START_CREATOR|%1', '*');").arg(user));
			break;
		}
		case CallAutomationState::ClientAutoJoining: {
			appendLog("Client Auto-Join started. Waiting seamlessly for active calls...", "#D19A66");
			m_browserPanel->runJavaScript("window.postMessage('START_CLIENT_USER', '*');");
			break;
		}
		case CallAutomationState::ClientAnonJoining: {
			QString link = m_callControlPanel->getInviteLink();
			appendLog(QString("Navigating directly to invite link: %1").arg(link), "#B0B0B0");
			m_browserPanel->loadUrl(link);
			break;
		}
		case CallAutomationState::Idle:
			appendLog("Automation halted.", "#B0B0B0");
			break;
	}
}

void MainWindow::onToggleCreatorAutoHost(bool active)
{
	changeCallState(active ? CallAutomationState::CreatorHosting : CallAutomationState::Idle);
}

void MainWindow::onToggleClientAutoJoin(bool active)
{
	changeCallState(active ? CallAutomationState::ClientAutoJoining : CallAutomationState::Idle);
}

void MainWindow::onTriggerClientAnonJoin()
{
	if (m_callControlPanel->getInviteLink().isEmpty() || !m_callControlPanel->getInviteLink().startsWith("http"))
	{
		appendLog("Please paste a valid VK Call link in the box first.", "#E06C75");
		return;
	}
	changeCallState(CallAutomationState::ClientAnonJoining);
}

void MainWindow::onPageLoaded(bool ok)
{
	if (!ok)
		return;

	if (m_callState == CallAutomationState::CreatorHosting && m_serviceState == ServiceState::RunningCreator)
	{
		QString user = m_callControlPanel->getTargetUser();
		m_browserPanel->runJavaScript(QString("window.postMessage('START_CREATOR|%1', '*');").arg(user));
	}
	else if (m_callState == CallAutomationState::ClientAutoJoining && m_serviceState == ServiceState::RunningClient)
	{
		m_browserPanel->runJavaScript("window.postMessage('START_CLIENT_USER', '*');");
	}
	else if (m_callState == CallAutomationState::ClientAnonJoining && m_serviceState == ServiceState::RunningClient)
	{
		m_browserPanel->runJavaScript("window.postMessage('START_CLIENT_ANON', '*');");
		m_callState = CallAutomationState::Idle;
	}
}

void MainWindow::appendLog(const QString& msg, const QString& color)
{
	m_logView->append(QString("<span style='color:#5C6370;'>[%1]</span> <span style='color:%2;'>%3</span>")
						  .arg(QTime::currentTime().toString("HH:mm:ss"), color, msg));
}

} // namespace APP::UI

#include "main_window.hpp"

#include "creator_server.hpp"
#include "relay_server.hpp"

#include <QSplitter>
#include <QTime>
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

	connect(m_browserPanel, &BrowserPanel::logRequested, this, [this](auto msg, auto c) {
		appendLog(msg, c);
	});
	connect(m_browserPanel, &BrowserPanel::pageLoaded, this, &MainWindow::onPageLoaded);
	connect(m_browserPanel, &BrowserPanel::callLinkExtracted, this, [this](const QString& link) {
		appendLog("Call Link Extracted! You can copy it now.", "#6A8759");
		m_callControlPanel->setInviteLink(link);
	});

	connect(SERVER::RelayServer::instance().get(), &SERVER::RelayServer::logMessage, this, [this](auto msg) {
		appendLog("[Relay] " + msg, "#6897BB");
	});
	connect(SERVER::CreatorServer::instance().get(), &SERVER::CreatorServer::logMessage, this, [this](auto msg, bool isDebug) {
		if (isDebug && !m_settingsPanel->isDebugMode())
			return;
		appendLog("[Creator] " + msg, isDebug ? "#7E9CD8" : "#C678DD");
	});

	m_settingsPanel->loadSettings();
}

MainWindow::~MainWindow()
{
	SERVER::RelayServer::instance()->stop();
	SERVER::CreatorServer::instance()->stop();
}

void MainWindow::onModeChanged(int mode)
{
	m_callControlPanel->updateVisibilityForMode(mode);
	m_browserPanel->setupProfileForMode(mode);
}

void MainWindow::onToggleServer()
{
	if (m_isServerRunning)
	{
		SERVER::RelayServer::instance()->stop();
		SERVER::CreatorServer::instance()->stop();
		m_isServerRunning = false;
		m_settingsPanel->setServerRunningState(false);
		appendLog("Service stopped.", "#B0B0B0");
	}
	else
	{
		bool success = false;
		if (m_settingsPanel->getMode() == 0)
		{
			success = SERVER::RelayServer::instance()->start(m_settingsPanel->getWsPort(), m_settingsPanel->getSocksPort());
		}
		else
		{
			success = SERVER::CreatorServer::instance()->start(m_settingsPanel->getWsPort());
		}

		if (success)
		{
			m_isServerRunning = true;
			m_settingsPanel->setServerRunningState(true);
			m_browserPanel->injectHooks(m_settingsPanel->getWsPort());
			appendLog("Service started. Hooks injected.", "#6A8759");
		}
		else
		{
			appendLog("[ERROR] Failed to start service. Check port availability.", "#CC666E");
		}
	}
}

void MainWindow::onToggleCreatorAutoHost(bool active)
{
	m_isCreatorAutoHostActive = active;
	if (active)
	{
		QString user = m_callControlPanel->getTargetUser();
		appendLog("Creator Auto-Host started. Maintaining call...", "#B0B0B0");
		m_browserPanel->runJavaScript(QString("window.postMessage('START_CREATOR|%1', '*');").arg(user));
	}
	else
	{
		appendLog("Creator Auto-Host stopped.", "#B0B0B0");
	}
}

void MainWindow::onToggleClientAutoJoin(bool active)
{
	m_isClientAutoJoinActive = active;
	if (active)
	{
		appendLog("Client Auto-Join started. Waiting for active calls on /calls?section=current...", "#B0B0B0");
		m_browserPanel->runJavaScript("window.postMessage('START_CLIENT_USER', '*');");
	}
	else
	{
		appendLog("Client Auto-Join stopped.", "#B0B0B0");
	}
}

void MainWindow::onTriggerClientAnonJoin()
{
	QString link = m_callControlPanel->getInviteLink();
	if (link.isEmpty() || !link.startsWith("http"))
	{
		appendLog("Please paste a valid VK Call link in the box first.", "#CC666E");
		return;
	}
	appendLog("Navigating to invite link...", "#B0B0B0");
	m_isClientAnonJoinActive = true;
	m_browserPanel->loadUrl(link);
}

void MainWindow::onPageLoaded(bool ok)
{
	if (!ok)
		return;

	if (m_isCreatorAutoHostActive && m_settingsPanel->getMode() == 1)
	{
		QString user = m_callControlPanel->getTargetUser();
		m_browserPanel->runJavaScript(QString("window.postMessage('START_CREATOR|%1', '*');").arg(user));
	}

	if (m_isClientAutoJoinActive && m_settingsPanel->getMode() == 0)
	{
		m_browserPanel->runJavaScript("window.postMessage('START_CLIENT_USER', '*');");
	}

	if (m_isClientAnonJoinActive && m_settingsPanel->getMode() == 0)
	{
		m_browserPanel->runJavaScript("window.postMessage('START_CLIENT_ANON', '*');");
		m_isClientAnonJoinActive = false;
	}
}

void MainWindow::appendLog(const QString& msg, const QString& color)
{
	m_logView->append(QString("<span style='color:#5C6370;'>[%1]</span> <span style='color:%2;'>%3</span>")
						  .arg(QTime::currentTime().toString("HH:mm:ss"), color, msg));
}

} // namespace APP::UI

#include "browser_panel.hpp"

#include "custom_web_page.hpp"
#include "generated/vk_auto_call.hpp"
#include "generated/vk_hook.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWebEngineHistory>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

namespace APP::UI
{

BrowserPanel::BrowserPanel(QWidget* parent) : QWidget(parent)
{
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	auto* navLayout = new QHBoxLayout();
	m_btnBack		= new QPushButton("◁");
	m_btnForward	= new QPushButton("▷");
	m_btnRefresh	= new QPushButton("↻");
	m_urlInput		= new QLineEdit();
	auto* btnGo		= new QPushButton("Go");

	navLayout->addWidget(m_btnBack);
	navLayout->addWidget(m_btnForward);
	navLayout->addWidget(m_btnRefresh);
	navLayout->addWidget(m_urlInput);
	navLayout->addWidget(btnGo);

	m_webView = new QWebEngineView();
	layout->addLayout(navLayout);
	layout->addWidget(m_webView);

	m_clientProfile = new QWebEngineProfile("ClientProfile", this);
	m_clientProfile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

	m_creatorProfile = new QWebEngineProfile("CreatorProfile", this);
	m_creatorProfile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

	connect(btnGo, &QPushButton::clicked, this, [this] {
		loadUrl(m_urlInput->text());
	});
	connect(m_urlInput, &QLineEdit::returnPressed, this, [this] {
		loadUrl(m_urlInput->text());
	});

	connect(m_btnBack, &QPushButton::clicked, m_webView, &QWebEngineView::back);
	connect(m_btnForward, &QPushButton::clicked, m_webView, &QWebEngineView::forward);
	connect(m_btnRefresh, &QPushButton::clicked, m_webView, &QWebEngineView::reload);

	connect(m_webView, &QWebEngineView::urlChanged, this, [this](const QUrl& url) {
		m_urlInput->setText(url.toString());
		m_btnBack->setEnabled(m_webView->history()->canGoBack());
		m_btnForward->setEnabled(m_webView->history()->canGoForward());
	});

	connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
		m_btnBack->setEnabled(m_webView->history()->canGoBack());
		m_btnForward->setEnabled(m_webView->history()->canGoForward());
		emit pageLoaded(ok);
	});
}

BrowserPanel::~BrowserPanel()
{
	if (m_webView)
		m_webView->setPage(nullptr);
}

void BrowserPanel::setupProfileForMode(int mode)
{
	QWebEngineProfile* targetProfile = (mode == 0) ? m_clientProfile : m_creatorProfile;
	if (m_webView->page() && m_webView->page()->profile() == targetProfile)
		return;

	auto* customPage = new WEBHOOK::CustomWebPage(targetProfile, m_webView);

	connect(customPage, &WEBHOOK::CustomWebPage::hookLogReceived, this, [this](const QString& msg) {
		emit logRequested(msg, "#CC7832");
	});
	connect(customPage, &WEBHOOK::CustomWebPage::callLinkExtracted, this, [this](const QString& link) {
		emit callLinkExtracted(link);
	});

	m_webView->setPage(customPage);
	loadUrl("https://vk.com/calls");
}

void BrowserPanel::loadUrl(const QString& urlStr)
{
	QUrl url = QUrl::fromUserInput(urlStr.trimmed());
	if (url.isValid())
		m_webView->setUrl(url);
}

void BrowserPanel::injectHooks(uint16_t wsPort)
{
	auto* profile = m_webView->page()->profile();
	profile->scripts()->clear();

	QWebEngineScript rtcHook;

	rtcHook.setSourceCode(QString::fromUtf8(RESOURCES::VK_HOOK_JS.data()).arg(wsPort));
	rtcHook.setInjectionPoint(QWebEngineScript::DocumentCreation);
	rtcHook.setWorldId(QWebEngineScript::MainWorld);
	rtcHook.setRunsOnSubFrames(true);

	QWebEngineScript autoCallHook;
	autoCallHook.setSourceCode(QString::fromUtf8(RESOURCES::VK_AUTO_CALL_JS.data()));
	autoCallHook.setInjectionPoint(QWebEngineScript::DocumentReady);
	autoCallHook.setWorldId(QWebEngineScript::MainWorld);
	autoCallHook.setRunsOnSubFrames(true);

	profile->scripts()->insert(rtcHook);
	profile->scripts()->insert(autoCallHook);

	m_webView->reload();
}

void BrowserPanel::runJavaScript(const QString& js)
{
	if (m_webView->page())
		m_webView->page()->runJavaScript(js);
}

} // namespace APP::UI

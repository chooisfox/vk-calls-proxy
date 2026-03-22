#pragma once
#include <QLineEdit>
#include <QPushButton>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QWidget>

namespace APP::UI
{

class BrowserPanel : public QWidget
{
	Q_OBJECT
public:
	explicit BrowserPanel(QWidget* parent = nullptr);
	~BrowserPanel() override;

	void setupProfileForMode(int mode);
	void loadUrl(const QString& url);
	void injectHooks(uint16_t wsPort);
	void runJavaScript(const QString& js);

signals:
	void logRequested(const QString& msg, const QString& color);
	void callLinkExtracted(const QString& link);
	void pageLoaded(bool ok);

private:
	QWebEngineView* m_webView;
	QLineEdit*		m_urlInput;
	QPushButton*	m_btnBack;
	QPushButton*	m_btnForward;
	QPushButton*	m_btnRefresh;

	QWebEngineProfile* m_clientProfile {nullptr};
	QWebEngineProfile* m_creatorProfile {nullptr};
};

} // namespace APP::UI

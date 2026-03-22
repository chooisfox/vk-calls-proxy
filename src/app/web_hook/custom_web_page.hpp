#pragma once
#include <QWebEnginePage>

namespace APP::WEBHOOK
{

class CustomWebPage final : public QWebEnginePage
{
	Q_OBJECT
public:
	explicit CustomWebPage(QWebEngineProfile* profile, QObject* parent = nullptr);

protected:
	void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineID, const QString& sourceID) override;
	QWebEnginePage* createWindow(WebWindowType type) override;

signals:
	void hookLogReceived(const QString& msg);
	void callLinkExtracted(const QString& link);
};

} // namespace APP::WEBHOOK

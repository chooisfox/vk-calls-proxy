#include "custom_web_page.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QWebEnginePermission>
#endif

namespace APP::WEBHOOK
{

CustomWebPage::CustomWebPage(QWebEngineProfile* profile, QObject* parent) : QWebEnginePage(profile, parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	connect(this, &QWebEnginePage::permissionRequested, this, [](QWebEnginePermission permission) {
		permission.grant();
	});
#else
	connect(this, &QWebEnginePage::featurePermissionRequested, this, [this](const QUrl& securityOrigin, QWebEnginePage::Feature feature) {
		setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionGrantedByUser);
	});
#endif
}

void CustomWebPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel /*level*/,
											 const QString& message,
											 int /*lineID*/,
											 const QString& /*sourceID*/)
{
	if (message.startsWith("[VK-CALL-LINK] "))
	{
		QString link = message.mid(15).trimmed();
		emit	callLinkExtracted(link);
	}
	else if (message.contains("[VK-HOOK]") || message.contains("[VK-AUTO]"))
	{
		emit hookLogReceived(message);
	}
}

QWebEnginePage* CustomWebPage::createWindow(WebWindowType /*type*/)
{
	return this;
}

} // namespace APP::WEBHOOK

#include "application.hpp"

namespace APP::APPLICATION
{

Application::Application(int& argc, char** argv) : QApplication(argc, argv)
{
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
}

int Application::exec()
{
	m_mainWindow.show();
	return QApplication::exec();
}

} // namespace APP::APPLICATION

#include "application.hpp"

#include "option_manager.hpp"
#include "settings_manager.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

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
	if (m_mainWindow)
	{
		m_mainWindow->show();
	}

	return QApplication::exec();
}

bool Application::initializeManagers(int argc, char** argv)
{
	auto settings_manager = UTILS::SettingsManager::instance();
	auto option_manager	  = UTILS::OptionManager::instance();

	auto			   timestamp = std::chrono::system_clock::now();
	std::time_t		   now_tt	 = std::chrono::system_clock::to_time_t(timestamp);
	std::tm			   tm		 = *std::localtime(&now_tt);
	std::ostringstream oss;
	oss << std::put_time(&tm, "%c %Z");

	settings_manager->set_setting("application.last-launch", oss.str());
	settings_manager->save_settings();

	option_manager->add_option("h,help", "Prints this help menu.");
	option_manager->add_option("d,debug", "Enable debug traffic logging.");
	option_manager->add_option("start", "Automatically start the service on launch.");

	option_manager->add_option<std::string>("m,mode", "Mode: 'client' or 'creator'");
	option_manager->add_option<uint16_t>("w,ws-port", "WebSocket port");
	option_manager->add_option<uint16_t>("s,socks-port", "SOCKS5 port (Client only)");

	option_manager->parse_options(argc, argv);

	if (option_manager->has_option("h"))
	{
		option_manager->log_help();
		return false;
	}

	if (option_manager->has_option("d"))
	{
		settings_manager->set_setting<bool>("application.debug", true);
	}

	if (option_manager->has_option("m"))
	{
		std::string mode = option_manager->get_option<std::string>("m");
		if (mode == "creator")
			settings_manager->set_setting<int64_t>("application.mode", 1);
		else if (mode == "client")
			settings_manager->set_setting<int64_t>("application.mode", 0);
	}

	if (option_manager->has_option("w"))
	{
		uint16_t port = option_manager->get_option<uint16_t>("w");
		settings_manager->set_setting<int64_t>("server.ws_port", port);
		settings_manager->set_setting<int64_t>("client.ws_port", port);
	}

	if (option_manager->has_option("s"))
	{
		settings_manager->set_setting<int64_t>("client.socks_port", option_manager->get_option<uint16_t>("s"));
	}

	m_mainWindow = std::make_unique<UI::MainWindow>();

	if (option_manager->has_option("start"))
	{
		triggerAutoStart();
	}

	return true;
}

void Application::triggerAutoStart()
{
	if (m_mainWindow)
	{
		m_mainWindow->triggerAutoStart();
	}
}

} // namespace APP::APPLICATION

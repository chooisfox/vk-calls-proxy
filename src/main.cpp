#include "application.hpp"
#include "settings_manager.hpp"

int main(int argc, char *argv[])
{
	UTILS::SettingsManager::instance();

	APP::APPLICATION::Application app(argc, argv);
	return app.exec();
}

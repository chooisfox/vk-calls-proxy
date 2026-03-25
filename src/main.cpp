#include "application.hpp"

int main(int argc, char** argv)
{
	APP::APPLICATION::Application app(argc, argv);

	if (app.initializeManagers(argc, argv))
	{
		return app.exec();
	}
	else
	{
		return 0;
	}
}

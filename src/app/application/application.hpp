#pragma once
#include "main_window.hpp"

#include <QApplication>

namespace APP::APPLICATION
{

class Application final : public QApplication
{
public:
	Application(int& argc, char** argv);
	int exec();

private:
	UI::MainWindow m_mainWindow;
};

} // namespace APP::APPLICATION

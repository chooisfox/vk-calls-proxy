#pragma once
#include "main_window.hpp"

#include <QApplication>
#include <memory>

namespace APP::APPLICATION
{

class Application final : public QApplication
{
public:
	Application(int& argc, char** argv);
	int exec();

	bool initializeManagers(int argc, char** argv);
	void triggerAutoStart();

private:
	std::unique_ptr<UI::MainWindow> m_mainWindow;
};

} // namespace APP::APPLICATION

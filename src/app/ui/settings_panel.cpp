#include "settings_panel.hpp"

#include "settings_manager.hpp"

#include <QHBoxLayout>
#include <QLabel>

namespace APP::UI
{

SettingsPanel::SettingsPanel(QWidget* parent) : QWidget(parent)
{
	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_modeCombo = new QComboBox();
	m_modeCombo->addItem("Client Mode (Relay / SOCKS5)", 0);
	m_modeCombo->addItem("Creator Mode (TCP Server)", 1);

	m_wsPortSpin = new QSpinBox();
	m_wsPortSpin->setRange(1024, 65535);
	m_wsPortSpin->setPrefix("WS Port: ");

	m_socksPortSpin = new QSpinBox();
	m_socksPortSpin->setRange(1024, 65535);
	m_socksPortSpin->setPrefix("SOCKS5 Port: ");

	m_cbDebugMode = new QCheckBox("Debug Traffic");

	m_btnToggleServer = new QPushButton("Start Service");
	m_btnToggleServer->setStyleSheet("background-color: #383A42; color: #D3D3D3; border: 1px solid #5C5C42; padding: 4px 15px;");

	layout->addWidget(new QLabel("Mode:"));
	layout->addWidget(m_modeCombo);
	layout->addWidget(m_wsPortSpin);
	layout->addWidget(m_socksPortSpin);
	layout->addWidget(m_cbDebugMode);
	layout->addWidget(m_btnToggleServer);
	layout->addStretch();

	connect(m_modeCombo, &QComboBox::currentIndexChanged, this, [this](int mode) {
		m_socksPortSpin->setVisible(mode == 0);
		onSettingsChanged();
		emit modeChanged(mode);
	});
	connect(m_wsPortSpin, &QSpinBox::editingFinished, this, &SettingsPanel::onSettingsChanged);
	connect(m_socksPortSpin, &QSpinBox::editingFinished, this, &SettingsPanel::onSettingsChanged);
	connect(m_cbDebugMode, &QCheckBox::toggled, this, &SettingsPanel::debugToggled);
	connect(m_btnToggleServer, &QPushButton::clicked, this, &SettingsPanel::toggleServerRequested);
}

void SettingsPanel::loadSettings()
{
	auto settings = UTILS::SettingsManager::instance();
	int	 mode	  = static_cast<int>(settings->get_setting<int64_t>("application.mode", 0));

	m_modeCombo->blockSignals(true);
	m_modeCombo->setCurrentIndex(mode);
	m_modeCombo->blockSignals(false);
	m_socksPortSpin->setVisible(mode == 0);

	m_wsPortSpin->setValue(static_cast<int>(settings->get_setting<int64_t>(mode == 0 ? "client.ws_port" : "server.ws_port", 9000)));
	m_socksPortSpin->setValue(static_cast<int>(settings->get_setting<int64_t>("client.socks_port", 1080)));
	emit modeChanged(mode);
}

void SettingsPanel::onSettingsChanged()
{
	auto settings = UTILS::SettingsManager::instance();
	int	 mode	  = m_modeCombo->currentIndex();
	settings->set_setting<int64_t>("application.mode", mode);
	if (mode == 0)
	{
		settings->set_setting<int64_t>("client.ws_port", m_wsPortSpin->value());
		settings->set_setting<int64_t>("client.socks_port", m_socksPortSpin->value());
	}
	else
	{
		settings->set_setting<int64_t>("server.ws_port", m_wsPortSpin->value());
	}
	settings->save_settings();
}

int SettingsPanel::getMode() const
{
	return m_modeCombo->currentIndex();
}
uint16_t SettingsPanel::getWsPort() const
{
	return m_wsPortSpin->value();
}
uint16_t SettingsPanel::getSocksPort() const
{
	return m_socksPortSpin->value();
}
bool SettingsPanel::isDebugMode() const
{
	return m_cbDebugMode->isChecked();
}

void SettingsPanel::setServerRunningState(bool running)
{
	if (running)
	{
		m_btnToggleServer->setText("Stop Service");
	}
	else
	{
		m_btnToggleServer->setText("Start Service");
	}
}
} // namespace APP::UI

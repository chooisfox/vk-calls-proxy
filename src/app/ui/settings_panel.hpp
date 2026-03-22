#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

namespace APP::UI
{

class SettingsPanel : public QWidget
{
	Q_OBJECT
public:
	explicit SettingsPanel(QWidget* parent = nullptr);
	void loadSettings();

	int		 getMode() const;
	uint16_t getWsPort() const;
	uint16_t getSocksPort() const;
	bool	 isDebugMode() const;

	void setServerRunningState(bool running);

signals:
	void modeChanged(int newMode);
	void toggleServerRequested();
	void debugToggled(bool enabled);

private slots:
	void onSettingsChanged();

private:
	QComboBox*	 m_modeCombo;
	QSpinBox*	 m_wsPortSpin;
	QSpinBox*	 m_socksPortSpin;
	QCheckBox*	 m_cbDebugMode;
	QPushButton* m_btnToggleServer;
};

} // namespace APP::UI

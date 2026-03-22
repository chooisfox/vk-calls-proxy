#include "call_control_panel.hpp"

#include <QHBoxLayout>

namespace APP::UI
{

CallControlPanel::CallControlPanel(QWidget* parent) : QWidget(parent)
{
	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_inviteLinkBox = new QLineEdit();
	m_inviteLinkBox->setPlaceholderText("Invite Link");
	m_inviteLinkBox->setStyleSheet("background-color: #2E2E2E; color: #E5C07B; font-weight: bold; border: 1px solid #5C5C42; padding: 4px;");

	m_inviteTargetBox = new QLineEdit();
	m_inviteTargetBox->setPlaceholderText("Clien app user (e.g. 101010101 or Имя Фамилия)");
	m_inviteTargetBox->setStyleSheet("background-color: #2E2E2E; color: #61AFEF; font-weight: bold; border: 1px solid #5C5C42; padding: 4px;");

	m_btnCreatorHost = new QPushButton("Create & Host Call");
	m_btnCreatorHost->setStyleSheet("background-color: #2E5C31; color: white; border: none; padding: 4px 15px; font-weight: bold;");

	m_btnClientAnonJoin = new QPushButton("Join as Anon (from Link)");
	m_btnClientAnonJoin->setStyleSheet("background-color: #314B70; color: white; border: none; padding: 4px 15px; font-weight: bold;");

	m_btnClientAutoJoin = new QPushButton("Auto-Join (Logged In)");
	m_btnClientAutoJoin->setStyleSheet("background-color: #703166; color: white; border: none; padding: 4px 15px; font-weight: bold;");

	layout->addWidget(m_inviteLinkBox);
	layout->addWidget(m_inviteTargetBox);
	layout->addWidget(m_btnCreatorHost);
	layout->addWidget(m_btnClientAnonJoin);
	layout->addWidget(m_btnClientAutoJoin);

	connect(m_btnCreatorHost, &QPushButton::clicked, this, [this]() {
		m_isCreatorHostActive = !m_isCreatorHostActive;
		setCreatorHostActive(m_isCreatorHostActive);
		emit toggleCreatorHostRequested(m_isCreatorHostActive);
	});

	connect(m_btnClientAutoJoin, &QPushButton::clicked, this, [this]() {
		m_isClientAutoJoinActive = !m_isClientAutoJoinActive;
		setClientAutoJoinActive(m_isClientAutoJoinActive);
		emit toggleClientAutoJoinRequested(m_isClientAutoJoinActive);
	});

	connect(m_btnClientAnonJoin, &QPushButton::clicked, this, &CallControlPanel::triggerClientAnonJoinRequested);
}

void CallControlPanel::updateVisibilityForMode(int mode)
{
	if (mode == 0)
	{
		m_btnCreatorHost->setVisible(false);
		m_inviteTargetBox->setVisible(false);
		m_btnClientAnonJoin->setVisible(true);
		m_btnClientAutoJoin->setVisible(true);
		m_inviteLinkBox->setReadOnly(false);
	}
	else
	{
		m_btnCreatorHost->setVisible(true);
		m_inviteTargetBox->setVisible(true);
		m_btnClientAnonJoin->setVisible(false);
		m_btnClientAutoJoin->setVisible(false);
		m_inviteLinkBox->setReadOnly(true);

		setClientAutoJoinActive(false);
	}
}

void CallControlPanel::setInviteLink(const QString& link)
{
	m_inviteLinkBox->setText(link);
}
QString CallControlPanel::getInviteLink() const
{
	return m_inviteLinkBox->text().trimmed();
}
QString CallControlPanel::getTargetUser() const
{
	return m_inviteTargetBox->text().trimmed();
}

void CallControlPanel::setCreatorHostActive(bool active)
{
	m_isCreatorHostActive = active;
	if (active)
	{
		m_btnCreatorHost->setText("Stop Hosting");
		m_btnCreatorHost->setStyleSheet("background-color: #8C2121; color: white; border: none; padding: 4px 15px; font-weight: bold;");
	}
	else
	{
		m_btnCreatorHost->setText("Create & Host Call");
		m_btnCreatorHost->setStyleSheet("background-color: #2E5C31; color: white; border: none; padding: 4px 15px; font-weight: bold;");
	}
}

void CallControlPanel::setClientAutoJoinActive(bool active)
{
	m_isClientAutoJoinActive = active;
	if (active)
	{
		m_btnClientAutoJoin->setText("Stop Auto-Join");
		m_btnClientAutoJoin->setStyleSheet("background-color: #8C2121; color: white; border: none; padding: 4px 15px; font-weight: bold;");
	}
	else
	{
		m_btnClientAutoJoin->setText("Auto-Join (Logged In)");
		m_btnClientAutoJoin->setStyleSheet("background-color: #703166; color: white; border: none; padding: 4px 15px; font-weight: bold;");
	}
}

} // namespace APP::UI

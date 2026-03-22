#pragma once
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace APP::UI
{

class CallControlPanel : public QWidget
{
	Q_OBJECT
public:
	explicit CallControlPanel(QWidget* parent = nullptr);

	void	updateVisibilityForMode(int mode);
	void	setInviteLink(const QString& link);
	QString getInviteLink() const;
	QString getTargetUser() const;

	void setCreatorHostActive(bool active);
	void setClientAutoJoinActive(bool active);

signals:
	void toggleCreatorHostRequested(bool active);
	void toggleClientAutoJoinRequested(bool active);
	void triggerClientAnonJoinRequested();

private:
	QLineEdit*	 m_inviteLinkBox;
	QLineEdit*	 m_inviteTargetBox;
	QPushButton* m_btnCreatorHost;
	QPushButton* m_btnClientAnonJoin;
	QPushButton* m_btnClientAutoJoin;

	bool m_isCreatorHostActive {false};
	bool m_isClientAutoJoinActive {false};
};

} // namespace APP::UI

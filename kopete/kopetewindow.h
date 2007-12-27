/*
    kopetewindow.h  -  Kopete Main Window

    Copyright (c) 2001-2002 by Duncan Mac-Vicar Prett <duncan@kde.org>
    Copyright (c) 2001-2002 by Stefan Gehn            <metz AT gehn.net>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef KOPETEWINDOW_H
#define KOPETEWINDOW_H

// Qt includes
#include <QtGui/QLabel>

// KDE includes
#include <kxmlguiwindow.h>

class QCloseEvent;
class QEvent;
class QShowEvent;

class KMenu;

namespace Kopete
{
	class Identity;
	class Account;
	class Contact;
	class Plugin;
}

/**
 * @author Duncan Mac-Vicar P. <duncan@kde.org>
 */
class KopeteWindow : public KXmlGuiWindow
{
	Q_OBJECT

public:
	explicit KopeteWindow ( QWidget *parent = 0 );
	~KopeteWindow();

	virtual bool eventFilter( QObject* o, QEvent* e );

protected:
	virtual void closeEvent( QCloseEvent *ev );
	virtual void leaveEvent( QEvent* ev );
	virtual void showEvent( QShowEvent* ev );

private slots:
	void slotToggleShowAllOfflineEmpty( bool toggled );
	void slotToggleShowOfflineUsers();
	void slotToggleShowEmptyGroups();
	void slotConfigChanged();
	void slotConfNotifications();
	void slotConfToolbar();
	void slotUpdateToolbar();
	void slotConfGlobalKeys();
	void slotShowHide();
	void slotToggleAway();


	void setStatusMessage( const QString & );

	/**
	 * Checks if the mousecursor is in the contact list.
	 * If not, the window will be hidden.
	 */
	void slotAutoHide();

	/**
	 * This slot will apply settings that change the
	 * contact list's appearance. Only autohiding is
	 * handled here at the moment
	 */
	void slotContactListAppearanceChanged();

	/**
	 * This slot will set all the protocols to away
	 */
	void slotGlobalAway();
	void slotGlobalBusy();
	void slotGlobalAvailable();
	void slotSetInvisibleAll();
	void slotDisconnectAll();

	void slotQuit();

	/**
	 * Get a notification when a plugin is loaded, so we can merge
	 * XMLGUI cruft
	 */
	void slotPluginLoaded( Kopete::Plugin *p );

	/**
	 * Get a notification when an identity is created, so we can add a status bar
	 * icon
	 * @param identity the registered identity 
	 */
	void slotIdentityRegistered( Kopete::Identity *identity );

	/**
	 * Cleanup the status bar icon when the identity is destroyed
	 * @param identity the unregistered identity
	 */
	void slotIdentityUnregistered( const Kopete::Identity *identity );

	/**
	 * The status icon got changed, update it.
	 * @param identity the identity that has changed
	 */
	void slotIdentityStatusIconChanged( Kopete::Identity *identity );

	/**
	 * Show a in-place widget for an identity
	 */
	void slotIdentityStatusIconLeftClicked( Kopete::Identity *i,
		const QPoint &p );


	/**
	 * This is used to add the account to the "Add Contact" submenu
	 */
	void slotAccountRegistered( Kopete::Account *account );

	/**
	 * This is used to add the account to the "Add Contact" submenu
	 */
	void slotAccountUnregistered( const Kopete::Account *account );

	void slotTrayAboutToShowMenu(KMenu *);

	/**
	 * Show the Add Contact wizard
	 */
	void showAddContactDialog( Kopete::Account * );

	/**
	 * Show the Export Contacts wizards
	 */
	void showExportDialog();

	/**
	 * Enable the Connect All and Disconnect All buttons here
	 * along with connecting the accountRegistered and accountUnregistered
	 * signals.
	 */
	void slotAllPluginsLoaded();

	/**
	 * Protected slot to setup the Set Global Status Message menu.
	 */
	void slotBuildStatusMessageMenu();
	void slotStatusMessageSelected( QAction *action );
	void slotNewStatusMessageEntered();

	/**
	 * Show the set global status message menu when clicking on the icon in the status bar.
	 */
	void slotGlobalStatusMessageIconClicked( const QPoint &position );

	/**
	 * Extracts protocolId and accountId from the single QString argument signalled by a QSignalMapper,
	 * get the account, and call showAddContactDialog.
	 * @param accountIdentifer QString of protocolId and accountId, concatenated with QChar( 0xE000 )
	 * We need both to uniquely identify an account, but QSignalMapper only emits one QString.
	 */
	void slotAddContactDialogInternal( const QString & accountIdentifier );

private:
	void initView();
	void initActions();
	void initSystray();
	void loadOptions();
	void saveOptions();

	void makeTrayToolTip();
	void startAutoHideTimer();

	virtual bool queryClose();
	virtual bool queryExit();

private:
	class Private;
	Private *d;
};


class GlobalStatusMessageIconLabel : public QLabel
{
      Q_OBJECT
public:
      GlobalStatusMessageIconLabel(QWidget *parent = 0);

protected:
      void mouseReleaseEvent(QMouseEvent *event);

signals:
      void iconClicked(const QPoint &position);

};

#endif
// vim: set noet ts=4 sts=4 sw=4:
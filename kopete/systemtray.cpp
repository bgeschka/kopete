/*
    systemtray.cpp  -  Kopete Tray Dock Icon

    Copyright (c) 2002      by Nick Betcher           <nbetcher@kde.org>
    Copyright (c) 2002-2003 by Martijn Klingens       <klingens@kde.org>
    Copyright (c) 2003      by Olivier Goffart        <ogoffart@tiscalinet.be>

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

#include "systemtray.h"

#include <qtimer.h>
#include <qtooltip.h>
#include <qregexp.h>

#include <kwin.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include "kopeteuiglobal.h"
#include "kopetemessagemanagerfactory.h"
#include "kopeteballoon.h"
#include "kopeteprefs.h"
#include "kopetemetacontact.h"
#include "kopeteaccount.h"
#include "kopeteaccountmanager.h"


KopeteSystemTray* KopeteSystemTray::s_systemTray = 0L;

KopeteSystemTray* KopeteSystemTray::systemTray( QWidget *parent, const char* name )
{
	if( !s_systemTray )
		s_systemTray = new KopeteSystemTray( parent, name );

	return s_systemTray;
}

KopeteSystemTray::KopeteSystemTray(QWidget* parent, const char* name)
	: KSystemTray(parent,name)
{
//	kdDebug(14010) << "Creating KopeteSystemTray" << endl;
	QToolTip::add( this, kapp->aboutData()->shortDescription() );

	mIsBlinkIcon = false;
	mIsBlinking = false;
	mBlinkTimer = new QTimer(this, "mBlinkTimer");

	mKopeteIcon = loadIcon("kopete");

	connect(mBlinkTimer, SIGNAL(timeout()), this, SLOT(slotBlink()));
	connect(KopeteMessageManagerFactory::factory() , SIGNAL(newEvent(KopeteEvent*)),
		this, SLOT(slotNewEvent(KopeteEvent*)));
	connect(KopetePrefs::prefs(), SIGNAL(saved()), this, SLOT(slotConfigChanged()));

	connect(KopeteAccountManager::manager(),
		SIGNAL(accountOnlineStatusChanged(KopeteAccount *,
		const KopeteOnlineStatus &, const KopeteOnlineStatus &)),
	this, SLOT(slotReevaluateAccountStates()));

	//setPixmap(mKopeteIcon);
	slotReevaluateAccountStates();
	slotConfigChanged();

	m_balloon=0l;
}

KopeteSystemTray::~KopeteSystemTray()
{
//	kdDebug(14010) << "[KopeteSystemTray] ~KopeteSystemTray" << endl;
//	delete mBlinkTimer;
	Kopete::UI::Global::setSysTrayWId( 0 );
}

void KopeteSystemTray::mousePressEvent( QMouseEvent *me )
{
	if (
		(me->button() == QEvent::MidButton || me->button() == QEvent::LeftButton) &&
		mIsBlinking )
	{
		mouseDoubleClickEvent( me );
		return;
	}

	KSystemTray::mousePressEvent( me );
}

void KopeteSystemTray::mouseDoubleClickEvent( QMouseEvent *me )
{
	if ( !mIsBlinking )
	{
		KSystemTray::mousePressEvent( me );
	}
	else
	{
		if(!mEventList.isEmpty())
			mEventList.first()->apply();
	}
}

void KopeteSystemTray::contextMenuAboutToShow( KPopupMenu *me )
{
	//kdDebug(14010) << k_funcinfo << "Called." << endl;
	emit aboutToShowMenu( me );
}

void KopeteSystemTray::startBlink( const QString &icon )
{
	startBlink( KGlobal::iconLoader()->loadIcon( icon , KIcon::Panel ) );
}

void KopeteSystemTray::startBlink( const QPixmap &icon )
{
	mBlinkIcon = icon;
	if ( mBlinkTimer->isActive() == false )
	{
		mIsBlinkIcon = true;
		mIsBlinking = true;
		mBlinkTimer->start( 1000, false );
	}
	else
	{
		mBlinkTimer->stop();
		mIsBlinkIcon = true;
		mIsBlinking = true;
		mBlinkTimer->start( 1000, false );
	}
}

void KopeteSystemTray::startBlink( const QMovie &movie )
{
	//kdDebug( 14010 ) << k_funcinfo << "starting movie." << endl;
	const_cast<QMovie &>( movie ).unpause();
	setMovie( movie );
	mIsBlinking = true;
}

void KopeteSystemTray::startBlink()
{
	if ( mMovie.isNull() )
		mMovie = KGlobal::iconLoader()->loadMovie( QString::fromLatin1( "newmessage" ), KIcon::Panel );

	startBlink( mMovie );
}

void KopeteSystemTray::stopBlink()
{
	if ( movie() )
		kdDebug( 14010 ) << k_funcinfo << "stopping movie." << endl;
	else if ( mBlinkTimer->isActive() )
		mBlinkTimer->stop();

	if ( !mMovie.isNull() )
		mMovie.pause();

	mIsBlinkIcon = false;
	mIsBlinking = false;
	//setPixmap( mKopeteIcon );
	slotReevaluateAccountStates();
}

void KopeteSystemTray::slotBlink()
{
	setPixmap( mIsBlinkIcon ? mKopeteIcon : mBlinkIcon );

	mIsBlinkIcon = !mIsBlinkIcon;
}

void KopeteSystemTray::slotNewEvent( KopeteEvent *event )
{
	mEventList.append( event );
	connect(event, SIGNAL(done(KopeteEvent*)),
		this, SLOT(slotEventDone(KopeteEvent*)));

	if( event->message().manager() != 0 )
	{
		if( event->message().manager()->account() )
		{
			if( !event->message().manager()->account()->isAway() ||
				KopetePrefs::prefs()->soundIfAway() )
			{
				kdDebug(14000) << k_funcinfo << "adding a balloon" << endl;
				addBalloon();
			}
			else
			{
				kdDebug(14000) << k_funcinfo << "Supressing balloon, account is away" << endl;
			}
		}
	}
	else
		kdDebug(14000) << k_funcinfo << "NULL message().manager()!" << endl;

	// tray animation
	if ( KopetePrefs::prefs()->trayflashNotify() )
		startBlink();
}

void KopeteSystemTray::slotEventDone(KopeteEvent *event)
{
	bool current= event==mEventList.first();
	mEventList.remove(event);

	if(current && m_balloon)
	{
		m_balloon->deleteLater();
		m_balloon=0l;
		if(!mEventList.isEmpty())
			addBalloon();
	}

	if(mEventList.isEmpty())
		stopBlink();
}

void KopeteSystemTray::addBalloon()
{
	/*kdDebug(14010) << k_funcinfo <<
		m_balloon << ":" << KopetePrefs::prefs()->showTray() <<
		":" << KopetePrefs::prefs()->balloonNotify()
		<< ":" << !mEventList.isEmpty() << endl;*/

	if( !m_balloon && KopetePrefs::prefs()->showTray() && KopetePrefs::prefs()->balloonNotify() && !mEventList.isEmpty() )
	{
		KopeteMessage msg = mEventList.first()->message();

		if ( msg.from() )
		{
			//kdDebug(14010) << k_funcinfo << "Orig msg text=" << msg.parsedBody() << endl;
			QString msgText = squashMessage( msg );
			//kdDebug(14010) << k_funcinfo << "New msg text=" << msgText << endl;

			QString msgFrom;
			if( msg.from()->metaContact() )
				msgFrom = msg.from()->metaContact()->displayName();
			else
				msgFrom = msg.from()->displayName();

			m_balloon = new KopeteBalloon(
				i18n( "<qt><nobr><b>New Message from %1:</b></nobr><br><nobr>\"%2\"</nobr></qt>" )
					.arg( msgFrom, msgText ), QString::null );
			connect(m_balloon, SIGNAL(signalBalloonClicked()), mEventList.first() , SLOT(apply()));
			connect(m_balloon, SIGNAL(signalButtonClicked()), mEventList.first() , SLOT(apply()));
			connect(m_balloon, SIGNAL(signalIgnoreButtonClicked()), mEventList.first() , SLOT(ignore()));
			m_balloon->setAnchor(mapToGlobal(pos()));
			m_balloon->show();
			KWin::setOnAllDesktops(m_balloon->winId(), true);
		}
	}
}

void KopeteSystemTray::slotConfigChanged()
{
//	kdDebug(14010) << k_funcinfo << "called." << endl;
	if ( KopetePrefs::prefs()->showTray() )
		show();
	else
		hide(); // for users without kicker or a similar docking app
}

void KopeteSystemTray::slotReevaluateAccountStates()
{
	//kdDebug(14010) << k_funcinfo << endl;
	bool bOnline = false;
	bool bAway = false;
	bool bOffline = false;
	KopeteContact *c = 0;

	for (QPtrListIterator<KopeteAccount> it(KopeteAccountManager::manager()->accounts()); it.current(); ++it)
	{
		c = it.current()->myself();
		if (!c)
			continue;

		if (c->onlineStatus().status() == KopeteOnlineStatus::Online)
		{
			bOnline = true; // at least one contact is online
		}
		else if (c->onlineStatus().status() == KopeteOnlineStatus::Away)
		{
			bAway = true; // at least one contact is away
		}
		else // this account must be offline (or unknown, which I don't know how to handle)
		{
			bOffline = true;
		}
	}

	if (!bOnline && !bAway && !bOffline) // special case, no accounts defined (yet)
		bOffline = true;

	if (bAway)
	{
		if (!bOnline && !bOffline) // none online and none offline -> all away
			setPixmap(loadIcon("kopete_all_away"));
		else
			setPixmap(loadIcon("kopete_some_away"));
	}
	else if(bOnline)
	{
		/*if(bOffline) // at least one offline and at least one online -> some accounts online
			setPixmap(loadIcon("kopete_some_online"));
		else*/ // none offline and none away -> all online
			setPixmap(mKopeteIcon);
	}
	else // none away and none online -> all offline
	{
		//kdDebug(14010) << k_funcinfo << "All Accounts offline!" << endl;
		setPixmap(loadIcon("kopete_offline"));
	}
}


QString KopeteSystemTray::squashMessage( const KopeteMessage& msg )
{
	QString msgText = msg.parsedBody();

	QRegExp rx( "(<a.*>((http://)?(.+))</a>)" );
	rx.setMinimal( true );
	if ( rx.search( msgText ) == -1 )
	{
		// no URLs in text, just pick the first 30 chars of
		// the plain text if necessary
		msgText = msg.plainBody();
		if( msgText.length() > 30 )
			msgText = msgText.left( 30 ) + QString::fromLatin1( " ..." );
	}
	else
	{
		QString plainText = msg.plainBody();
		if ( plainText.length() > 30 )
		{
			QString fullUrl = rx.cap( 2 );
			QString shorterUrl;
			if ( fullUrl.length() > 30 )
			{
				QString urlWithoutProtocol = rx.cap( 4 );
				shorterUrl = urlWithoutProtocol.left( 27 )
						+ QString::fromLatin1( "... " );
			}
			else
			{
				shorterUrl = fullUrl.left( 27 )
						+ QString::fromLatin1( "... " );
			}
			// remove message text
			msgText = QString::fromLatin1( "... " ) +
					rx.cap( 1 ) +
					QString::fromLatin1( " ..." );
			// find last occurrence of URL (the one inside the <a> tag)
			int revUrlOffset = msgText.findRev( fullUrl );
			msgText.replace( revUrlOffset,
						fullUrl.length(), shorterUrl );
		}
	}
	return msgText;
}

#include "systemtray.moc"
// vim: set noet ts=4 sts=4 sw=4:

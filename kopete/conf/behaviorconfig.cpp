/*
    behaviorconfig.cpp  -  Kopete Look Feel Config

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

#include "behaviorconfig.h"

#include <qcheckbox.h>
//#include <qdir.h>
#include <qlayout.h>
#include <qhbuttongroup.h>
#include <qspinbox.h>
//#include <qslider.h>

#include <kdebug.h>
#include <klocale.h>
#include <knotifydialog.h>
#include <kpushbutton.h>
/*
#include <klineedit.h>
#include <kcolorcombo.h>
#include <kcolorbutton.h>
#include <klineeditdlg.h>
#include <khtmlview.h>
#include <khtml_part.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>
#include <kurlrequesterdlg.h>
#include <kfontdialog.h>
#include <ktrader.h>
#include <klibloader.h>
#include <kiconview.h>
*/
#include "kopeteprefs.h"
#include "kopeteaway.h"
#include "kopeteawayconfigui.h"

#include "behaviorconfig_general.h"
#include "behaviorconfig_chat.h"

#include <qtabwidget.h>

BehaviorConfig::BehaviorConfig(QWidget * parent) :
	ConfigModule (
		i18n("Behavior"),
		i18n("Here You Can Personalize Kopete"),
		"package_settings", // TODO: find or create a icon that fits
		parent )
{
	(new QVBoxLayout(this))->setAutoAdd(true);
	mBehaviorTabCtl = new QTabWidget(this, "mBehaviorTabCtl");

	// "General" TAB ============================================================
	mPrfsGeneral = new BehaviorConfig_General(mBehaviorTabCtl);
	connect(mPrfsGeneral->configSound, SIGNAL(clicked()),
		this, SLOT(slotConfigSound()));
	connect(mPrfsGeneral->mShowTrayChk, SIGNAL(toggled(bool)),
		this, SLOT(slotShowTrayChanged(bool)));
	mBehaviorTabCtl->addTab(mPrfsGeneral, i18n("&General"));

	// "Away" TAB ===============================================================
	mAwayConfigUI = new KopeteAwayConfigUI(mBehaviorTabCtl);
	mBehaviorTabCtl->addTab(mAwayConfigUI, i18n("&Away Settings"));

	// "Chat" TAB ===============================================================
	mPrfsChat = new BehaviorConfig_Chat(mBehaviorTabCtl);
	mBehaviorTabCtl->addTab(mPrfsChat, i18n("&Chat"));
}


void BehaviorConfig::save()
{
	kdDebug(14000) << k_funcinfo << "called." << endl;

	KopetePrefs *p = KopetePrefs::prefs();
	KopeteAway *ka = KopeteAway::getInstance();

	// "General" TAB ============================================================
	p->setShowTray(mPrfsGeneral->mShowTrayChk->isChecked());
	p->setStartDocked(mPrfsGeneral->mStartDockedChk->isChecked());
	p->setUseQueue(mPrfsGeneral->mUseQueueChk->isChecked());
	p->setTrayflashNotify(mPrfsGeneral->mTrayflashNotifyChk->isChecked());
	p->setBalloonNotify(mPrfsGeneral->mBalloonNotifyChk->isChecked());
	p->setSoundIfAway(mPrfsGeneral->mSoundIfAwayChk->isChecked());
	p->setTreeView(mPrfsGeneral->mTreeContactList->isChecked());
	p->setShowOffline(mPrfsGeneral->mShowOfflineUsers->isChecked());
	p->setSortByGroup(mPrfsGeneral->mSortByGroup->isChecked());
	p->setGreyIdleMetaContacts(mPrfsGeneral->mGreyIdleMetaContacts->isChecked());


	// "Away" TAB ===============================================================
	p->setNotifyAway( mAwayConfigUI->mNotifyAway->isChecked());
	ka->setUseAutoAway(mAwayConfigUI->mUseAutoAway->isChecked());
	ka->setAutoAwayTimeout(mAwayConfigUI->mAutoAwayTimeout->value() * 60);
	ka->setGoAvailable(mAwayConfigUI->mGoAvailable->isChecked());
	ka->save();


	// "Chat" TAB ===============================================================
	p->setRaiseMsgWindow(mPrfsChat->cb_RaiseMsgWindowChk->isChecked());
	p->setShowEvents(mPrfsChat->cb_ShowEventsChk->isChecked());
	p->setHighlightEnabled(mPrfsChat->highlightEnabled->isChecked());
	p->setChatWindowPolicy(mPrfsChat->chatWindowGroup->id(
			mPrfsChat->chatWindowGroup->selected()));
	p->setInterfacePreference(mPrfsChat->interfaceGroup->id(
			mPrfsChat->interfaceGroup->selected()) );
	p->setChatViewBufferSize(mPrfsChat->mChatViewBufferSize->value());

	// disconnect or else we will end up in an endless loop
	p->save();
}

void BehaviorConfig::reopen()
{
	kdDebug(14000) << k_funcinfo << "called" << endl;

	KopetePrefs *p = KopetePrefs::prefs();

	// "General" TAB ============================================================
	mPrfsGeneral->mShowTrayChk->setChecked( p->showTray() );
	mPrfsGeneral->mStartDockedChk->setChecked( p->startDocked() );
	mPrfsGeneral->mUseQueueChk->setChecked( p->useQueue() );
	mPrfsGeneral->mTrayflashNotifyChk->setChecked ( p->trayflashNotify() );
	mPrfsGeneral->mBalloonNotifyChk->setChecked ( p->balloonNotify() );
	mPrfsGeneral->mSoundIfAwayChk->setChecked( p->soundIfAway() );
	slotShowTrayChanged( mPrfsGeneral->mShowTrayChk->isChecked() );
	mPrfsGeneral->mTreeContactList->setChecked( p->treeView() );
	mPrfsGeneral->mSortByGroup->setChecked( p->sortByGroup() );
	mPrfsGeneral->mShowOfflineUsers->setChecked( p->showOffline() );
	mPrfsGeneral->mGreyIdleMetaContacts->setChecked( p->greyIdleMetaContacts() );


	// "Away" TAB ===============================================================
	mAwayConfigUI->updateView();
	mAwayConfigUI->mNotifyAway->setChecked( p->notifyAway() );


	// "Chat" TAB ===============================================================
	mPrfsChat->cb_RaiseMsgWindowChk->setChecked(p->raiseMsgWindow());
	mPrfsChat->cb_ShowEventsChk->setChecked(p->showEvents());
	mPrfsChat->highlightEnabled->setChecked(p->highlightEnabled());
	mPrfsChat->chatWindowGroup->setButton(p->chatWindowPolicy());
	mPrfsChat->interfaceGroup->setButton(p->interfacePreference());
	mPrfsChat->mChatViewBufferSize->setValue(p->chatViewBufferSize());
}


void BehaviorConfig::slotConfigSound()
{
	KNotifyDialog::configure(this);
}

void BehaviorConfig::slotShowTrayChanged(bool check)
{
	mPrfsGeneral->mStartDockedChk->setEnabled(check);
	mPrfsGeneral->mTrayflashNotifyChk->setEnabled(check);
	mPrfsGeneral->mBalloonNotifyChk->setEnabled(check);
}

#include "behaviorconfig.moc"
// vim: set noet ts=4 sts=4 sw=4:

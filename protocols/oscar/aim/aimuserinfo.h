/*
    oscaruserinfo.h  -  Oscar Protocol Plugin

    Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#ifndef AIMUSERINFO_H
#define AIMUSERINFO_H

#include <kdialogbase.h>
#include "aiminfobase.h"
#include "oscarsocket.h"

class KTextEdit;
class OscarAccount;
class AIMContact;
class AIMAccount;
class KHTMLPart;

class AIMUserInfoDialog : public KDialogBase
{
	Q_OBJECT
	public:
		AIMUserInfoDialog(AIMContact *c, AIMAccount *acc, bool modal,
			QWidget *parent, const char* name);
		~AIMUserInfoDialog();

	private:
		AIMAccount *mAccount;
		AIMContact *mContact;
		AIMUserInfoWidget *mMainWidget;
		KTextBrowser *userInfoView;
		KTextEdit *userInfoEdit;

	private slots:
		void slotSaveClicked();
		void slotCloseClicked();
		void slotUpdateClicked();
		void slotUpdateProfile();
		void slotUrlClicked(const QString&);
		void slotMailClicked(const QString&, const QString&);

	signals:
//		void updateNickname(const QString &);
		void closing();
};

#endif


/*************************************************************************
 * Copyright <2007>  <Michael Zanetti> <michael_zanetti@gmx.net>         *
 *                                                                       *
 * This program is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU General Public License as        *
 * published by the Free Software Foundation; either version 2 of        *
 * the License or (at your option) version 3 or any later version        *
 * accepted by the membership of KDE e.V. (or its successor approved     *
 * by the membership of KDE e.V.), which shall act as a proxy            *
 * defined in Section 14 of version 3 of the license.                    *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *************************************************************************/ 

/**
  * @author Michael Zanetti
  */

#include "privkeypopup.h"

#include <kwindowsystem.h>

PrivKeyPopup::PrivKeyPopup( QWidget *parent ):KDialog( parent ){
	QWidget *widget = new QWidget( this );
	//KWindowSystem::setState(widget->winId(), NET::StaysOnTop );
	ui.setupUi( widget );
	setMainWidget( widget );
	setCaption( i18n( "Please wait" ) );
	setButtons( KDialog::None );

	ui.lIcon->setPixmap( KIcon( "dialog-password" ).pixmap( 48, 48 ) );
}

PrivKeyPopup::~PrivKeyPopup(){
}


void PrivKeyPopup::setCloseLock( bool locked ){
	closeLock = locked;
}

void PrivKeyPopup::closeEvent( QCloseEvent *e ){
	if( closeLock ){
		e->ignore();
	} else {
		e->accept();
	}
}

#include "privkeypopup.moc"

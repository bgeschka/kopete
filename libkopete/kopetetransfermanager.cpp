/*
    kopetetransfermanager.cpp

    Copyright (c) 2002-2003 by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2002-2003 by Richard Smith <kopete@metafoo.co.uk>

    Kopete    (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#include <klocale.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kmessagebox.h>
#include <kio/jobuidelegate.h>
#include <kuiserverjobtracker.h>

#include "kopetemetacontact.h"
#include "kopetecontact.h"
#include "kopetemessage.h"
#include "kopetechatsession.h"
#include "kopeteuiglobal.h"

#include "kopetetransfermanager.h"
#include "kopetefileconfirmdialog.h"

/***************************
 *  Kopete::FileTransferInfo *
 ***************************/

Kopete::FileTransferInfo::FileTransferInfo(  Kopete::Contact *contact, const QString& file, const unsigned long size, const QString &recipient, KopeteTransferDirection di, const unsigned int id, QString internalId, const QPixmap &preview)
{
	mContact = contact;
	mFile = file;
	mId = id;
	mSize = size;
	mRecipient = recipient;
	m_intId= internalId;
	mDirection= di;
	mPreview = preview;
}

/***************************
 *     Kopete::Transfer      *
 ***************************/


Kopete::Transfer::Transfer( const Kopete::FileTransferInfo &kfti, const QString &localFile, bool showProgressInfo)
	: KIO::Job(), mInfo(kfti)
{
	this->setUiDelegate(new KIO::JobUiDelegate());
	if(showProgressInfo)
		KIO::getJobTracker()->registerJob(this);

	KUrl targ; targ.setPath( localFile );
	init( targ, showProgressInfo );
}

Kopete::Transfer::Transfer( const Kopete::FileTransferInfo &kfti, const Kopete::Contact *contact, bool showProgressInfo)
	: KIO::Job(), mInfo(kfti)
{
	this->setUiDelegate(new KIO::JobUiDelegate());
	if(showProgressInfo)
		KIO::getJobTracker()->registerJob(this);

	// TODO: use mInfo.url().fileName() after move to protocol-aware filetransfers
	KUrl targ; targ.setPath( mInfo.file() );
	init( displayURL( contact, targ.fileName() ), showProgressInfo );
}

void Kopete::Transfer::init( const KUrl &target, bool showProgressInfo )
{
	mTarget = target;

	if( showProgressInfo )
		emitCopying( sourceURL(), destinationURL() );

	connect( this, SIGNAL( result( KJob* ) ), SLOT( slotResultEmitted() ) );

	ui()->setAutoErrorHandlingEnabled( true );
}

Kopete::Transfer::~Transfer()
{
}

KUrl Kopete::Transfer::displayURL( const Kopete::Contact *contact, const QString &file )
{
	KUrl url;
	url.setProtocol( QString::fromLatin1("kopete") );

	QString host;
	if( !contact )
		host = QString::fromLatin1("unknown origin");
	else if( contact->metaContact() )
		host = contact->metaContact()->displayName();
	else
		host = contact->contactId();
	url.setHost(host);

	// url.setPath( contact->protocol()->displayName() );

	url.setFileName( file );
	return url;
}

// TODO: add possibility of network file transfers;
//  call mInfo->url() not file()
KUrl Kopete::Transfer::sourceURL()
{
	if( mInfo.direction() == Kopete::FileTransferInfo::Incoming )
		return displayURL( mInfo.contact(), mInfo.file() );
	else
	{
		KUrl url; url.setPath( mInfo.file() );
		return url;
	}
}

KUrl Kopete::Transfer::destinationURL()
{
	return mTarget;
}

void Kopete::Transfer::emitCopying(const KUrl &src, const KUrl &dest)
{
    emit description(this, i18n("Copying"),
                     qMakePair(i18n("Source"), src.prettyUrl()),
                     qMakePair(i18n("Destination"), dest.prettyUrl()));
}

void Kopete::Transfer::slotProcessed(unsigned int bytes)
{
	emitPercent( bytes, mInfo.size() );
}

void Kopete::Transfer::slotComplete()
{
	showMessage( i18n("File transfer %1 completed. :)", mInfo.file() ) );
	emitResult();
}

void Kopete::Transfer::slotError( int error, const QString &errorText )
{
	setError(error);
	setErrorText(errorText);

	showMessage( i18n("File transfer %1 failed. :(", mInfo.file() ) );
	emitResult();
}

void Kopete::Transfer::slotResultEmitted()
{
	if( error() == KIO::ERR_USER_CANCELED )
	{
		showMessage( i18n("You cancelled file transfer %1", mInfo.file() ) );
		emit transferCanceled();
	}
}

void Kopete::Transfer::slotCancelled()
{
	showMessage( i18n("File transfer %1 cancelled.", mInfo.file() ) );
	emitResult();
	//slotError( KIO::ERR_ABORTED, i18n("File transfer cancelled.") );
}

bool Kopete::Transfer::showMessage( QString text )
{
	Kopete::ChatSession *cs = mInfo.contact()->manager();
	if (! cs)
		return false;

	Kopete::Message msg;
	msg.setPlainBody( text );
	cs->appendMessage( msg );
	return true;
}


/***************************
 *  Kopete::TransferManager  *
 ***************************/

Kopete::TransferManager* Kopete::TransferManager::transferManager()
{
	static TransferManager s(0L);
	return &s;
}

Kopete::TransferManager::TransferManager( QObject *parent ) : QObject( parent )
{
	nextID = 0;
}

Kopete::Transfer* Kopete::TransferManager::addTransfer(  Kopete::Contact *contact, const QString& file, const unsigned long size, const QString &recipient , Kopete::FileTransferInfo::KopeteTransferDirection di)
{
//	if (nextID != 0)
		nextID++;
	Kopete::FileTransferInfo info(contact, file, size, recipient,di,  nextID);
	Kopete::Transfer *trans = new Kopete::Transfer(info, contact);
	connect(trans, SIGNAL(result(KJob *)), this, SLOT(slotComplete(KJob *)));
	mTransfersMap.insert(nextID, trans);
	return trans;
}

void Kopete::TransferManager::slotAccepted(const Kopete::FileTransferInfo& info, const QString& filename)
{
	Kopete::Transfer *trans = new Kopete::Transfer(info, filename);
	connect(trans, SIGNAL(result(KJob *)), this, SLOT(slotComplete(KJob *)));
	mTransfersMap.insert(info.transferId(), trans);
	emit accepted(trans,filename);
}

int Kopete::TransferManager::askIncomingTransfer(  Kopete::Contact *contact, const QString& file, const unsigned long size, const QString& description, QString internalId, const QPixmap &preview)
{
//	if (nextID != 0)
		nextID++;

	QString dn= contact ? (contact->metaContact() ? contact->metaContact()->displayName() : contact->contactId()) : i18n("<unknown>");

	Kopete::FileTransferInfo info(contact, file, size, dn, Kopete::FileTransferInfo::Incoming , nextID , internalId, preview);

	//FIXME!!! this will not be deleted if it's still open when kopete exits
	KopeteFileConfirmDialog *diag= new KopeteFileConfirmDialog(info, description , 0 )  ;

	connect( diag, SIGNAL( accepted(const Kopete::FileTransferInfo&, const QString&)) , this, SLOT( slotAccepted(const Kopete::FileTransferInfo&, const QString&) ) );
	connect( diag, SIGNAL( refused(const Kopete::FileTransferInfo&)) , this, SIGNAL( refused(const Kopete::FileTransferInfo&) ) );
	diag->show();
	return nextID;
}

void Kopete::TransferManager::removeTransfer( unsigned int id )
{
	mTransfersMap.remove(id);
	//we don't need to delete the job, the job get deleted itself
}

void Kopete::TransferManager::slotComplete(KJob *job)
{
	Kopete::Transfer *transfer=dynamic_cast<Kopete::Transfer*>(job);
	if(!transfer)
		return;

	emit done(transfer);

	for( QMap<unsigned, Kopete::Transfer*>::Iterator it = mTransfersMap.begin();
	     it != mTransfersMap.end(); ++it )
	{
		if( it.value() == transfer )
		{
			removeTransfer(it.key());
			break;
		}
	}
}

void Kopete::TransferManager::sendFile( const KUrl &file, const QString &fname, unsigned long sz,
	 bool mustBeLocal,	QObject *sendTo, const char *slot )
{
	KUrl url = file;
	QString filename;
	unsigned int size = 0;

	//If the file location is null, then get it from a file open dialog
	if( !url.isValid() )
		url = KFileDialog::getOpenUrl( KUrl(), QString::fromLatin1("*"), 0l, i18n( "Kopete File Transfer" ));
	else
	{
		filename = fname;
		size = sz;
	}

	if( filename.isEmpty() )
		filename = url.fileName();

	if( size == 0 )
	{
		KFileItem finfo(KFileItem::Unknown, KFileItem::Unknown, url);
		size = (unsigned long)finfo.size();
	}

	if( !url.isEmpty() )
	{
		if( mustBeLocal && !url.isLocalFile() )
		{
			KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
				i18n( "Sorry, sending files which are not stored locally is not yet supported by this protocol.\n"
				"Please copy this file to your computer and try again." ) );
		}
		else
		{
			connect( this, SIGNAL(sendFile(const KUrl&, const QString&, unsigned int)), sendTo, slot );
			emit sendFile( url, filename, size );
			disconnect( this, SIGNAL(sendFile(const KUrl&, const QString&, unsigned int)), sendTo, slot );
		}
	}
}

#include "kopetetransfermanager.moc"

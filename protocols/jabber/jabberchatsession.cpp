/*
    jabberchatsession.cpp - Jabber Chat Session

    Copyright (c) 2004 by Till Gerken            <till@tantalo.net>

    Kopete    (c) 2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#include "jabberchatsession.h"

#include <qlabel.h>
#include <qimage.h>

#include <qfile.h>
#include <qicon.h>
#include <config-kopete.h>

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kicon.h>
#include <kactioncollection.h>
#include "kopetechatsessionmanager.h"
#include "kopetemessage.h"
#include "kopeteviewplugin.h"
#include "kopeteview.h"
#include "kopetemetacontact.h"

#include "jabberprotocol.h"
#include "jabberaccount.h"
#include "jabberclient.h"
#include "jabbercontact.h"
#include "jabberresource.h"
#include "jabberresourcepool.h"
#include "kioslave/jabberdisco.h"


JabberChatSession::JabberChatSession ( JabberProtocol *protocol, const JabberBaseContact *user,
											 Kopete::ContactPtrList others, const QString &resource )
											 : Kopete::ChatSession ( user, others, protocol )
{
	kDebug ( JABBER_DEBUG_GLOBAL ) << "New message manager for " << user->contactId ();

	// make sure Kopete knows about this instance
	Kopete::ChatSessionManager::self()->registerChatSession ( this );

	connect ( this, SIGNAL ( messageSent ( Kopete::Message &, Kopete::ChatSession * ) ),
			  this, SLOT ( slotMessageSent ( Kopete::Message &, Kopete::ChatSession * ) ) );

	connect ( this, SIGNAL ( myselfTyping ( bool ) ), this, SLOT ( slotSendTypingNotification ( bool ) ) );

	connect ( this, SIGNAL ( onlineStatusChanged(Kopete::Contact*, const Kopete::OnlineStatus&, const Kopete::OnlineStatus& ) ), this, SLOT ( slotUpdateDisplayName () ) );

	// check if the user ID contains a hardwired resource,
	// we'll have to use that one in that case
	XMPP::Jid jid = user->rosterItem().jid() ;

	mResource = jid.resource().isEmpty () ? resource : jid.resource ();
	slotUpdateDisplayName ();

#ifdef SUPPORT_JINGLE
	KAction *jabber_voicecall = new KAction( i18n("Voice call" ), "voicecall", 0, members().getFirst(), SLOT(voiceCall ()), actionCollection(), "jabber_voicecall" );

	setComponentData(protocol->componentData());
	jabber_voicecall->setEnabled( false );

	
	Kopete::ContactPtrList chatMembers = members ();
	if ( chatMembers.first () )
	{
		// Check if the current contact support Voice calls, also honor lock by default.
		// FIXME: we should use the active ressource
		JabberResource *bestResource = account()->resourcePool()->  bestJabberResource( static_cast<JabberBaseContact*>(chatMembers.first())->rosterItem().jid() );
		if( bestResource && bestResource->features().canVoice() )
		{
			jabber_voicecall->setEnabled( true );
		}
	}

#endif

	KAction* sendFile = new KAction( this );
	sendFile->setIcon( KIcon( "attach" ) );
	sendFile->setText( i18n( "Send File" ) );
	QObject::connect(sendFile, SIGNAL( triggered( bool ) ), SLOT( slotSendFile() ));
	actionCollection()->addAction( "jabberSendFile", sendFile );
	setXMLFile("jabberchatui.rc");

}

JabberChatSession::~JabberChatSession( )
{
	JabberAccount * a = dynamic_cast<JabberAccount *>(Kopete::ChatSession::account ());
	if( !a ) //When closing kopete, the account is partially destroyed already,  dynamic_cast return 0
		return;
	if ( a->configGroup()->readEntry ("SendEvents", true) &&
			 a->configGroup()->readEntry ("SendGoneEvent", true) )
		sendNotification( Gone );
}


void JabberChatSession::slotUpdateDisplayName ()
{
	kDebug ( JABBER_DEBUG_GLOBAL ) ;

	Kopete::ContactPtrList chatMembers = members ();

	// make sure we do have members in the chat
	if ( !chatMembers.first () )
		return;

	XMPP::Jid jid = static_cast<JabberBaseContact*>(chatMembers.first())->rosterItem().jid();

	if ( !mResource.isEmpty () )
		jid.setResource ( mResource );

	QString statusText = i18nc("a contact's online status in parenthesis.", " (%1)",
							  chatMembers.first()->onlineStatus().description() );
	if ( jid.resource().isEmpty () )
		setDisplayName ( chatMembers.first()->metaContact()->displayName () + statusText );
	else
		setDisplayName ( chatMembers.first()->metaContact()->displayName () + '/' + jid.resource () + statusText );

}

const JabberBaseContact *JabberChatSession::user () const
{

	return static_cast<const JabberBaseContact *>(Kopete::ChatSession::myself());

}

JabberAccount *JabberChatSession::account () const
{

	return static_cast<JabberAccount *>(Kopete::ChatSession::account ());

}

const QString &JabberChatSession::resource () const
{

	return mResource;

}

void JabberChatSession::appendMessage ( Kopete::Message &msg, const QString &fromResource )
{

	mResource = fromResource;

	slotUpdateDisplayName ();
	Kopete::ChatSession::appendMessage ( msg );

	// We send the notifications for Delivered and Displayed events. More granular management
	// (ie.: send Displayed event when it is really displayed)
	// of these events would require changes in the chatwindow API.
	
	if ( account()->configGroup()->readEntry ("SendEvents", true) )
	{
		if ( account()->configGroup()->readEntry ("SendDeliveredEvent", true) )
		{
			sendNotification( Delivered );
		}
		
		if ( account()->configGroup()->readEntry ("SendDisplayedEvent", true) )
		{
			sendNotification( Displayed );
		}
	}
}

void JabberChatSession::sendNotification( Event event )
{
	if ( !account()->isConnected () )
		return;

	XMPP::MsgEvent msg_event;
	XMPP::ChatState new_state;
	bool send_msg_event=false;
	bool send_state;
	
	switch(event)
	{
		case Delivered:
			send_msg_event=true;
			msg_event=DeliveredEvent;
			break;
		case Displayed:
			send_msg_event=true;
			msg_event=DisplayedEvent;
			break;
		case Composing:
			send_msg_event=true;
			msg_event=ComposingEvent;
			send_state=true;
			new_state=XMPP::StateComposing;
			break;
		case CancelComposing:
			send_msg_event=true;
			msg_event=CancelEvent;
			send_state=true;
			//new_state=XMPP::StatePaused; //paused give some bad notification on some client. while this mean in kopete that we stopped typing.
			new_state=XMPP::StateActive;
			break;
		case Inactive:
			send_state=true;
			new_state=XMPP::StateInactive;
			break;
		case Gone:
			send_state=true;
			new_state=XMPP::StateGone;
			break;
		default:
			break;
	}
	
	if(send_msg_event)
	{
		send_msg_event=false;
		foreach(Kopete::Contact *c , members())
		{
			JabberContact *contact=static_cast<JabberContact*>(c);
			if(contact->isContactRequestingEvent( msg_event ))
			{
				send_msg_event=true;
				break;
			}
		}
	}
/*	if(send_state)
	{
		send_state=false;
		foreach(JabberContact *contact , members())
		{	
			JabberContact *c;
			if(contact->feature.canChatState()  )
			{
				send_state=true;
				break;
			}
		}
	}*/
	
	if(send_state || send_msg_event )
	{
		// create JID for us as sender
		XMPP::Jid fromJid = static_cast<const JabberBaseContact*>(myself())->rosterItem().jid();
		fromJid.setResource ( account()->resource () );
	
		// create JID for the recipient
		JabberContact *recipient = static_cast<JabberContact*>(members().first());
		XMPP::Jid toJid = recipient->rosterItem().jid();
	
		// set resource properly if it has been selected already
		if ( !resource().isEmpty () )
			toJid.setResource ( resource () );

		XMPP::Message message;

		message.setFrom ( fromJid );
		message.setTo ( toJid );
		if(send_msg_event)
		{
			message.setEventId ( recipient->lastReceivedMessageId () );
			// store composing event depending on state
			message.addEvent ( msg_event );
		}
		if(send_state)
		{
			message.setChatState( new_state );
		}
		
		if (view() && view()->plugin()->pluginId() == "kopete_emailwindow" )
		{	
			message.setType ( "normal" );
		}
		else
		{	
			message.setType ( "chat" );
		}


		// send message
		account()->client()->sendMessage ( message );
	}
}

void JabberChatSession::slotSendTypingNotification ( bool typing )
{
	if ( !account()->configGroup()->readEntry ("SendEvents", true)
		|| !account()->configGroup()->readEntry("SendComposingEvent", true) ) 
		return;

	// create JID for us as sender
	XMPP::Jid fromJid = static_cast<const JabberBaseContact*>(myself())->rosterItem().jid();
	fromJid.setResource ( account()->configGroup()->readEntry( "Resource", QString() ) );

	kDebug ( JABBER_DEBUG_GLOBAL ) << "Sending out typing notification (" << typing << ") to all chat members.";

	typing ? sendNotification( Composing ) : sendNotification( CancelComposing );
}

void JabberChatSession::slotMessageSent ( Kopete::Message &message, Kopete::ChatSession * )
{

	if( account()->isConnected () )
	{
		XMPP::Message jabberMessage;
		JabberBaseContact *recipient = static_cast<JabberBaseContact*>(message.to().first());

		XMPP::Jid jid = static_cast<const JabberBaseContact*>(message.from())->rosterItem().jid();
		jid.setResource ( account()->configGroup()->readEntry( "Resource", QString() ) );
		jabberMessage.setFrom ( jid );

		XMPP::Jid toJid = recipient->rosterItem().jid();

		if( !resource().isEmpty () )
			toJid.setResource ( resource() );

		jabberMessage.setTo ( toJid );

		jabberMessage.setSubject ( message.subject () );
		jabberMessage.setTimeStamp ( message.timestamp () );

		if ( message.plainBody().indexOf ( "-----BEGIN PGP MESSAGE-----" ) != -1 )
		{
			/*
			 * This message is encrypted, so we need to set
			 * a fake body indicating that this is an encrypted
			 * message (for clients not implementing this
			 * functionality) and then generate the encrypted
			 * payload out of the old message body.
			 */

			// please don't translate the following string
			jabberMessage.setBody ( i18n ( "This message is encrypted." ) );

			QString encryptedBody = message.plainBody ();

			// remove PGP header and footer from message
			encryptedBody.truncate ( encryptedBody.length () - QString("-----END PGP MESSAGE-----").length () - 2 );
			encryptedBody = encryptedBody.right ( encryptedBody.length () - encryptedBody.indexOf ( "\n\n" ) - 2 );

			// assign payload to message
			jabberMessage.setXEncrypted ( encryptedBody );
        }
        else
        {
			// this message is not encrypted
			jabberMessage.setBody ( message.plainBody ());
			if (message.format() ==  Qt::RichText) 
			{
				JabberResource *bestResource = account()->resourcePool()->bestJabberResource(toJid);
				if( bestResource && bestResource->features().canXHTML() )
				{
					QString xhtmlBody = message.escapedBody();
					
					// According to JEP-0071 8.9  it is only RECOMMANDED to replace \n with <br/>
					//  which mean that some implementation (gaim 2 beta) may still think that \n are linebreak.  
					// and considered the fact that KTextEditor generate a well indented XHTML, we need to remove all \n from it
					//  see Bug 121627
					// Anyway, theses client that do like that are *WRONG*  considreded the example of jep-71 where there are lot of
					// linebreak that are not interpreted.  - Olivier 2006-31-03
					xhtmlBody.replace('\n',"");
					
					//&nbsp; is not a valid XML entity
					xhtmlBody.replace("&nbsp;" , "&#160;");
							
					xhtmlBody="<p "+ message.getHtmlStyleAttribute() +'>'+ xhtmlBody +"</p>";
					
					QDomDocument doc;
					doc.setContent(xhtmlBody, true);
					jabberMessage.setHTML( XMPP::HTMLElement( doc.documentElement() ) );
				}
        	}
		}

		// determine type of the widget and set message type accordingly
		// "kopete_emailwindow" is the default email Kopete::ViewPlugin.  If other email plugins
		// become available, either jabber will have to provide its own selector or libkopete will need
		// a better way of categorising view plugins.

		// FIXME: the view() is a speedy way to solve BUG:108389. A better solution is to be found
		// but I don't want to introduce a new bug during the bug hunt ;-).
		if (view() && view()->plugin()->pluginId() == "kopete_emailwindow" )
		{	
			jabberMessage.setType ( "normal" );
		}
		else
		{	
			jabberMessage.setType ( "chat" );
		}

		// add request for all notifications
		jabberMessage.addEvent( OfflineEvent );
		jabberMessage.addEvent( ComposingEvent );
		jabberMessage.addEvent( DeliveredEvent );
		jabberMessage.addEvent( DisplayedEvent );
		jabberMessage.setChatState( XMPP::StateActive );
		

        // send the message
		account()->client()->sendMessage ( jabberMessage );

		// append the message to the manager
		Kopete::ChatSession::appendMessage ( message );

		// tell the manager that we sent successfully
		messageSucceeded ();
	}
	else
	{
		account()->errorConnectFirst ();

		// FIXME: there is no messageFailed() yet,
		// but we need to stop the animation etc.
		messageSucceeded ();
	}

}

 void JabberChatSession::slotSendFile()
      {
              QList<Kopete::Contact*>contacts = members();
              static_cast<JabberContact *>(contacts.first())->sendFile();
      }

#include "jabberchatsession.moc"

// vim: set noet ts=4 sts=4 sw=4:
// kate: tab-width 4; replace-tabs off; space-indent off;
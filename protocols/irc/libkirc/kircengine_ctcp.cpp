/*
    kirc_ctcp.h - IRC Client

    Copyright (c) 2003      by Michel Hermier <michel.hermier@wanadoo.fr>
    Copyright (c) 2002      by Nick Betcher <nbetcher@kde.org>
    Copyright (c) 2003      by Jason Keirstead <jason@keirstead.org>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kircengine.h"
#include "kirctransferhandler.h"

#include <kextsock.h>

#include <qfileinfo.h>
#include <qregexp.h>

using namespace KIRC;

void Engine::registerCtcp()
{
//	CTCP Queries
	addCtcpQueryIrcMethod("ACTION",		&Engine::CtcpQuery_action,	-1,	-1,	"");
	addCtcpQueryIrcMethod("CLIENTINFO",	&Engine::CtcpQuery_clientInfo,	-1,	1,	"");
	addCtcpQueryIrcMethod("DCC",		&Engine::CtcpQuery_dcc,		4,	5,	"");
	addCtcpQueryIrcMethod("FINGER",		&Engine::CtcpQuery_finger,	-1,	0,	"");
	addCtcpQueryIrcMethod("PING",		&Engine::CtcpQuery_pingPong,	1,	1,	"");
	addCtcpQueryIrcMethod("SOURCE",		&Engine::CtcpQuery_source,	-1,	0,	"");
	addCtcpQueryIrcMethod("TIME",		&Engine::CtcpQuery_time,		-1,	0,	"");
	addCtcpQueryIrcMethod("USERINFO",	&Engine::CtcpQuery_userInfo,	-1,	0,	"");
	addCtcpQueryIrcMethod("VERSION",	&Engine::CtcpQuery_version,	-1,	0,	"");

//	CTCP Replies
	addCtcpReplyIrcMethod("ERRMSG",		&Engine::CtcpReply_errorMsg,	1,	-1,	"");
	addCtcpReplyIrcMethod("PING",		&Engine::CtcpReply_pingPong,	1,	1,	"");
	addCtcpReplyIrcMethod("VERSION",	&Engine::CtcpReply_version,	-1,	-1,	"");
}

// Normal order for a ctcp command:
// CtcpRequest_*
// CtcpQuery_*
// CtcpReply_* (if any)

/* Generic ctcp commnd for the /ctcp trigger */
void Engine::CtcpRequestCommand(const QString &contact, const QString &command)
{
	if(m_status == Connected)
	{
		writeCtcpQueryMessage(contact, QString::null, command);
//		emit ctcpCommandMessage( contact, command );
	}
}

void Engine::CtcpRequest_action(const QString &contact, const QString &message)
{
	if(m_status == Connected)
	{
		writeCtcpQueryMessage(contact, QString::null, "ACTION", message );

		if( Entity::isChannel(contact) )
			emit incomingAction(contact, m_Nickname, message);
		else
			emit incomingPrivAction(m_Nickname, contact, message);
	}
}

bool Engine::CtcpQuery_action(const Message &msg)
{
	QString target = msg.arg(0);
	if (target[0] == '#' || target[0] == '!' || target[0] == '&')
		emit incomingAction(target, msg.nickFromPrefix(), msg.ctcpMessage().ctcpRaw());
	else
		emit incomingPrivAction(msg.nickFromPrefix(), target, msg.ctcpMessage().ctcpRaw());
	return true;
}

/*
NO REPLY EXIST FOR THE CTCP ACTION COMMAND !
bool Engine::CtcpReply_action(const Message &msg)
{
}
*/

//	FIXME: the API can now answer to help commands.
bool Engine::CtcpQuery_clientInfo(const Message &msg)
{
	QString response = customCtcpMap[ QString::fromLatin1("clientinfo") ];
	if( !response.isNull() )
	{
		writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
					msg.ctcpMessage().command(), QString::null, response);
	}
	else
	{
		QString info = QString::fromLatin1("The following commands are supported, but "
			"without sub-command help: VERSION, CLIENTINFO, USERINFO, TIME, SOURCE, PING,"
			"ACTION.");

		writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
					msg.ctcpMessage().command(), QString::null, info);
	}
	return true;
}

void Engine::CtcpRequest_dcc(const QString &nickname, const QString &fileName, uint port, Transfer::Type type)
{
	if(	m_status != Connected ||
		m_sock->localAddress() == 0 ||
		m_sock->localAddress()->nodeName() == QString::null)
		return;

	switch(type)
	{
		case Transfer::Chat:
		{
			writeCtcpQueryMessage(nickname, QString::null,
				QString::fromLatin1("DCC"),
				Engine::join( QString::fromLatin1("CHAT"), QString::fromLatin1("chat"),
					m_sock->localAddress()->nodeName(), QString::number(port)
				)
			);
			break;
		}

		case Transfer::FileOutgoing:
		{
			QFileInfo file(fileName);
			QString noWhiteSpace = file.fileName();
			if (noWhiteSpace.contains(' ') > 0)
				noWhiteSpace.replace(QRegExp("\\s+"), "_");

			TransferServer *server = TransferHandler::self()->createServer(this, nickname, type, fileName, file.size());

			QString ip = m_sock->localAddress()->nodeName();
			QRegExp reg("^(\\d{1,3}).(\\d{1,3}).(\\d{1,3}).(\\d{1,3})$");
			if (!reg.exactMatch(ip))
			{
				kdDebug(14120) << "Not an ipv4:" << ip << endl;
				return;
			}

			// FIXME: This is very an ugly way to do it
			QString ipNumber = QString::number(	(Q_UINT32)reg.cap(1).toUShort()*(256*256*256)+
								reg.cap(2).toUShort()*(256*256)+
								reg.cap(3).toUShort()*256+
								reg.cap(4).toUShort());
			kdDebug(14120) << "Starting DCC file outgoing transfer." << endl;

			writeCtcpQueryMessage(nickname, QString::null,
				QString::fromLatin1("DCC"),
				Engine::join( QString::fromLatin1( "SEND" ), noWhiteSpace, ipNumber,
					QString::number( server->port() ), QString::number( file.size() )
				)
			);
			break;
		}

		case Transfer::FileIncoming:
		case Transfer::Unknown:
		default:
			break;
	}
}

bool Engine::CtcpQuery_dcc(const Message &msg)
{
	const Message &ctcpMsg = msg.ctcpMessage();
	QString dccCommand = ctcpMsg.arg(0).upper();

	if (dccCommand == QString::fromLatin1("CHAT"))
	{
		if(ctcpMsg.argsSize()!=4) return false;

		/* DCC CHAT type longip port
		 *
		 *  type   = Either Chat or Talk, but almost always Chat these days
		 *  longip = 32-bit Internet address of originator's machine
		 *  port   = Port on which the originator is waitng for a DCC chat
		 */
		bool okayHost, okayPort;
		// should ctctMsg.arg(1) be tested?
		QHostAddress address(ctcpMsg.arg(2).toUInt(&okayHost));
		unsigned int port = ctcpMsg.arg(3).toUInt(&okayPort);
		if (okayHost && okayPort)
		{
			kdDebug(14120) << "Starting DCC chat window." << endl;
			TransferHandler::self()->createClient(
				this, msg.nickFromPrefix(),
				address, port,
				Transfer::Chat );
			return true;
		}
	}
	else if (dccCommand == QString::fromLatin1("SEND"))
	{
		if(ctcpMsg.argsSize()!=5) return false;

		/* DCC SEND (filename) (longip) (port) (filesize)
		 *
		 *  filename = Name of file being sent
		 *  longip   = 32-bit Internet address of originator's machine
		 *  port     = Port on which the originator is waiitng for a DCC chat
		 *  filesize = Size of file being sent
		 */
		bool okayHost, okayPort, okaySize;
//		QFileInfo realfile(msg.arg(1));
		QHostAddress address(ctcpMsg.arg(2).toUInt(&okayHost));
		unsigned int port = ctcpMsg.arg(3).toUInt(&okayPort);
		unsigned int size = ctcpMsg.arg(4).toUInt(&okaySize);
		if (okayHost && okayPort && okaySize)
		{
			kdDebug(14120) << "Starting DCC send file transfert for file:" << ctcpMsg.arg(1) << endl;
			TransferHandler::self()->createClient(
				this, msg.nickFromPrefix(),
				address, port,
				Transfer::FileIncoming,
				ctcpMsg.arg(1), size );
			return true;
		}
	}
//	else
//		emit unknown dcc command signal
	return false;
}

/*
NO REPLY EXIST FOR THE CTCP DCC COMMAND !
bool Engine::CtcpReply_dcc(const Message &msg)
{
}
*/

bool Engine::CtcpReply_errorMsg(const Message &)
{
	// should emit one signal
	return true;
}

bool Engine::CtcpQuery_finger( const Message & /* msg */ )
{
	// To be implemented
	return true;
}

void Engine::CtcpRequest_pingPong(const QString &target)
{
	kdDebug(14120) << k_funcinfo << endl;

	timeval time;
	if (gettimeofday(&time, 0) == 0)
	{
		QString timeReply;

		if( Entity::isChannel(target) )
			timeReply = QString::fromLatin1("%1.%2").arg(time.tv_sec).arg(time.tv_usec);
		else
		 	timeReply = QString::number( time.tv_sec );

		writeCtcpQueryMessage(	target, QString::null, "PING", timeReply);
	}
}

bool Engine::CtcpQuery_pingPong(const Message &msg)
{
	writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), msg.ctcpMessage().arg(0));
	return true;
}

bool Engine::CtcpReply_pingPong( const Message &msg )
{
	timeval time;
	if (gettimeofday(&time, 0) == 0)
	{
		// FIXME: the time code is wrong for usec
		QString timeReply = QString::fromLatin1("%1.%2").arg(time.tv_sec).arg(time.tv_usec);
		double newTime = timeReply.toDouble();
		double oldTime = msg.suffix().section(' ',0, 0).toDouble();
		double difference = newTime - oldTime;
		QString diffString;

		if (difference < 1)
		{
			diffString = QString::number(difference);
			diffString.remove((diffString.find('.') -1), 2);
			diffString.truncate(3);
			diffString.append("milliseconds");
		}
		else
		{
			diffString = QString::number(difference);
			QString seconds = diffString.section('.', 0, 0);
			QString millSec = diffString.section('.', 1, 1);
			millSec.remove(millSec.find('.'), 1);
			millSec.truncate(3);
			diffString = QString::fromLatin1("%1 seconds, %2 milliseconds").arg(seconds).arg(millSec);
		}

		emit incomingCtcpReply(QString::fromLatin1("PING"), msg.nickFromPrefix(), diffString);

		return true;
	}

	return false;
}

bool Engine::CtcpQuery_source(const Message &msg)
{
	writeCtcpReplyMessage( msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), m_SourceString);
	return true;
}

bool Engine::CtcpQuery_time(const Message &msg)
{
	writeCtcpReplyMessage(	msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), QDateTime::currentDateTime().toString(),
				QString::null, false);
	return true;
}

bool Engine::CtcpQuery_userInfo(const Message &msg)
{
	QString response = customCtcpMap[ QString::fromLatin1("userinfo") ];
	if( !response.isNull() )
	{
		writeCtcpReplyMessage(msg.nickFromPrefix(), QString::null,
			msg.ctcpMessage().command(), QString::null, response);
	}
	else
	{
		writeCtcpReplyMessage( msg.nickFromPrefix(), QString::null,
				msg.ctcpMessage().command(), QString::null, m_UserString );
	}

	return true;
}

void Engine::CtcpRequest_version(const QString &target)
{
	writeCtcpQueryMessage(target, QString::null, "VERSION");
}

bool Engine::CtcpQuery_version(const Message &msg)
{
	QString response = customCtcpMap[ QString::fromLatin1("version") ];
	kdDebug(14120) << "Version check: " << response << endl;

	if (response.isNull())
		response = m_VersionString;

	writeCtcpReplyMessage(msg.nickFromPrefix(),
		msg.ctcpMessage().command() + " " + response);

	return true;
}

bool Engine::CtcpReply_version(const Message &msg)
{
	emit incomingCtcpReply(msg.ctcpMessage().command(), msg.nickFromPrefix(), msg.ctcpMessage().ctcpRaw());
	return true;
}

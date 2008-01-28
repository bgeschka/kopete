/*
    kircmessage.cpp - IRC Client

    Copyright (c) 2003-2007 by Michel Hermier <michel.hermier@gmail.com>

    Kopete    (c) 2003-2007 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kircmessage.h"

#include "kircconst.h"
#include "kirccontext.h"
#include "kircsocket.h"
#include "kircmessageutil.h"

#include <kdebug.h>

#include <QPointer>
#include <QSharedData>
#include <QTextCodec>

//QRegExp Message::sd->("^()\\r\\n$")
/*
#ifndef _IRC_STRICTNESS_
QRegExp Message::sd->IRCNumericCommand("^\\d{1,3}$");

// TODO: This regexp parsing is no good. It's slower than it needs to be, and
// is not codec-safe since QString requires a codec.
QRegExp Message::sd->IRCCommandType1(
	"^(?::([^ ]+) )?([A-Za-z]+|\\d{1,3})((?: [^ :][^ ]*)*) ?(?: :(.*))?$");
	// Extra end arg space check -------------------------^
#else // _IRC_STRICTNESS_
QRegExp Message::sd->IRCNumericCommand("^\\d{3,3}$");

QRegExp Message::sd->IRCCommandType1(
	"^(?::([^ ]+) )?([A-Za-z]+|\\d{3,3})((?: [^ :][^ ]*){0,13})(?: :(.*))?$");
QRegExp Message::sd->IRCCommandType2(
	"^(?::[[^ ]+) )?([A-Za-z]+|\\d{3,3})((?: [^ :][^ ]*){14,14})(?: (.*))?$");
#endif // _IRC_STRICTNESS_
*/

class KIrc::MessagePrivate
	: public QSharedData
{
public:
//	QByteArray line;

	QByteArray prefix;
	KByteArrayList args;
	QByteArray suffix;
};

using namespace KIrc;

Message::Message()
{
}

Message::Message(const QByteArray &prefix,
		 const KByteArrayList &args,
		 const QByteArray &suffix)
	: d(new KIrc::MessagePrivate())
{
	d->prefix = prefix;
	d->args = args;
	d->suffix = suffix;
}

/*
Message::Message(const QByteArray &line)
	: Event(Event::Message)
	, d(new Private())
{
	setLine(line);
}
*/
Message::Message(const Message &o)
	: d(o.d)
{
}

Message::~Message()
{
}

Message &Message::operator=(const Message &o)
{
//	Event::operator=(o);
	return *this;
}

Message Message::fromLine(const QByteArray &line, bool *ok)
{
	bool success = false;

	QByteArray prefix;
	KByteArrayList args;
	QByteArray suffix;

	// Match a regexp instead of the replace ...
//	remove the trailling \r\n if any(there must be in fact)
//	d->line.replace("\r\n","");
 
#ifdef __GNUC__
	#warning implement me: parsing
#endif
/*
	int token_start = 0;
	int token_end;
	while(token_endline.indexOf(' ') > 0)
	{
#ifndef KIRC_STRICT
//		eat empty tokens
#endif // KIRC_STRICT
	}
*/
	if (ok)
		*ok = success;

	if (success)
		return Message(prefix, args, suffix);
	else
		return Message();
}

QByteArray Message::toLine() const
{
	// FIXME: we need to do some escaping checks here.
	KByteArrayList list;

	if (!d->prefix.isEmpty())
		list.append(':' + d->prefix);

	foreach(const QByteArray arg, d->args)
	{
		if (arg.isEmpty())
			list.append("*");
		else
			list.append(arg);
	}

	// Append suffix when empty.
	if (!d->suffix.isNull())
		list.append(':' + d->suffix);

	return list.join(' ') + "\r\n";
}

QByteArray Message::prefix() const
{
	return d->prefix;
}

QString Message::prefix(QTextCodec *codec) const
{
	Q_ASSERT(codec);
	return codec->toUnicode(d->prefix);
}

Message &Message::operator << (const KByteArrayList::OptArg &arg)
{
	d->args << arg;
	return *this;
}

KByteArrayList Message::args() const
{
	return d->args;
}

#if 0
QString Message::command(QTextCodec *codec) const
{
	Q_ASSERT(codec);
	return codec->toUnicode(d->command);
}
#endif

QByteArray Message::argAt(int i) const
{
	// Using value here so that it check bounds.
	return d->args.value(i);
}

QByteArray Message::suffix() const
{
	return d->suffix;
}

QString Message::suffix(QTextCodec *codec) const
{
	Q_ASSERT(codec);
	return codec->toUnicode(d->suffix);
}

bool Message::isNumericReply() const
{
#ifdef __GNUC__
#warning FIXME
#endif
//      return d->IRCNumericCommand.exactMatch(d->command);
        return false;
}


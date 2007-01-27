/*
 oscarprotocol.h  -  Oscar Protocol Plugin

 Copyright (c) 2002 by Tom Linsky <twl6@po.cwru.edu>
 Copyright (c) 2005 by Matt Rogers <mattr@kde.org>
 Copyright (c) 2006 by Roman Jarosz <kedgedev@centrum.cz>

 Kopete    (c) 2002-2006 by the Kopete developers  <kopete-devel@kde.org>

 *************************************************************************
 *                                                                       *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 *************************************************************************
 */

#ifndef OSCARPROTOCOL_H
#define OSCARPROTOCOL_H

#include "kopeteprotocol.h"
#include "kopete_export.h"
#include "kopetecontactproperty.h"


class KOPETE_EXPORT OscarProtocol : public Kopete::Protocol
{
	Q_OBJECT

public:
	OscarProtocol( const KComponentData &instance, QObject *parent );
	virtual ~OscarProtocol();

	virtual Kopete::Contact *deserializeContact( Kopete::MetaContact *metaContact,
	                                             const QMap<QString, QString> &serializedData,
	                                             const QMap<QString, QString> &addressBookData );
	
	const Kopete::ContactPropertyTmpl awayMessage;
	const Kopete::ContactPropertyTmpl clientFeatures;
	const Kopete::ContactPropertyTmpl buddyIconHash;
	const Kopete::ContactPropertyTmpl contactEncoding;

};

#endif 
//kate: tab-width 4; indent-mode csands;

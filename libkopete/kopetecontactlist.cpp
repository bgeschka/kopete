/*
    kopetecontactlist.cpp - Kopete's Contact List backend

    Copyright (c) 2002 by Martijn Klingens       <klingens@kde.org>

	Copyright (c) 2002 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "kopetecontactlist.h"

#include "kopetemetacontact.h"

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kdebug.h>

KopeteContactList *KopeteContactList::s_contactList = 0L;

KopeteContactList *KopeteContactList::contactList()
{
	if( !s_contactList )
		s_contactList = new KopeteContactList;

	return s_contactList;
}

KopeteContactList::KopeteContactList()
: QObject( kapp, "KopeteContactList" )
{
}

KopeteContactList::~KopeteContactList()
{
}

KopeteMetaContact *KopeteContactList::findContact( const QString &contactId )
{
	QPtrListIterator<KopeteMetaContact> it( m_contacts );
	for( ; it.current(); ++it )
	{
		QPtrListIterator<KopeteContact> contactIt( it.current()->contacts() );
		for( ; contactIt.current(); ++contactIt )
		{
			if( contactIt.current()->id() == contactId )
				return it.current();
		}
	}

	// Contact not found, create a new meta contact
	KopeteMetaContact *c = new KopeteMetaContact();
	m_contacts.append( c );
	return c;
}

void KopeteContactList::loadXML()
{
	QString xml_filename;

	m_dom = new QDomDocument("ContactList");
	xml_filename = locateLocal("data","kopete/contacts.xml");

    /* No contacts */
	if ( xml_filename.isNull() )
		return ;

	QFile xml_file(xml_filename);
	xml_file.open(IO_ReadWrite);
	m_dom->setContent(&xml_file);

	QDomElement list = m_dom->documentElement();

	QDomNode nodel1;
	nodel1 = list.firstChild();

	while ( ! nodel1.isNull() )
	{
		QDomElement elementl1 = nodel1.toElement();

		if ( ! elementl1.isNull())
		{
			/* We have found a metacontact person */
			if ( elementl1.tagName() == "person" )
			{
				QString person_name = elementl1.attribute("name", "No Name");
				kdDebug() << "XML Reader: New Person" << person_name << endl;

				/* Now we have to find all contacts and metadata for this person */
				QDomNode nodel2;
				nodel2 = nodel1.firstChild();

    			while ( ! nodel2.isNull() )
				{
                    /* We try to convert it to an element */
					QDomElement elementl2 = nodel2.toElement();

                    /* Was it an element ? */
					if ( ! elementl2.isNull())
					{
						kdDebug() << "XML Reader: " << elementl2.tagName() << endl;
            			/* We have found a plugin contact */
						if ( elementl2.tagName() == "contact" )
						{
							QString contactid = elementl2.attribute("id", "Help!");
							QString protocol = elementl2.attribute("protocol", "Unknown");
							kdDebug() << "XML Reader: \tNew Contact ID:" << person_name << " Protocol: " << protocol << endl;

						}
						if ( elementl2.tagName() == "metadata" )
						{
							QString pluginid = elementl2.attribute("pluginid", "Ups");
							QString mdkey = elementl2.attribute("key", "No key");
							kdDebug() << "XML Reader: \tNew Metadata PluginID: " << pluginid << " Key: " << mdkey << endl;

						}
					}

					/* We go for the next contact, metadata, etc */
                	nodel2 = nodel2.nextSibling();
				}

			}

		}
		nodel1 = nodel1.nextSibling();
	}

	/* All ok! */
}

void KopeteContactList::saveXML()
{


}

QStringList KopeteContactList::meta_all()
{
	QStringList meta_contacts;
	QPtrListIterator<KopeteMetaContact> it( KopeteContactList::contactList()->m_contacts );
	for( ; it.current(); ++it )
	{
		meta_contacts.append( it.current()->displayName() );
	}
	return meta_contacts;
}

QStringList KopeteContactList::meta_status()
{
	QStringList meta_contacts;
	QPtrListIterator<KopeteMetaContact> it( KopeteContactList::contactList()->m_contacts );
	for( ; it.current(); ++it )
	{
		meta_contacts.append( QString ("%1 (%2)").arg( it.current()->displayName() ).arg( it.current()->statusString() ) );
	}
	return meta_contacts;
}

QStringList KopeteContactList::meta_reachable()
{
	QStringList meta_contacts;
	QPtrListIterator<KopeteMetaContact> it( KopeteContactList::contactList()->m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->isReachable() )
			meta_contacts.append( it.current()->displayName() );
	}
	return meta_contacts;
}

QStringList KopeteContactList::meta_online()
{
	QStringList meta_contacts;
	QPtrListIterator<KopeteMetaContact> it( KopeteContactList::contactList()->m_contacts );
	for( ; it.current(); ++it )
	{
		if ( it.current()->isOnline() )
			meta_contacts.append( it.current()->displayName() );
	}
	return meta_contacts;
}


#include "kopetecontactlist.moc"

// vim: set noet ts=4 sts=4 sw=4:


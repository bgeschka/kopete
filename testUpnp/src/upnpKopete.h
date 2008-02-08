/*
    upnpKopete.h -

    Copyright (c) 2007-2008 by Romain Castan      <romaincastan@gmail.com>
    Copyright (c) 2007-2008 by Bertrand Demay     <bertranddemay@gmail.com>
    Copyright (c) 2007-2008 by Julien Hubatzeck   <reineur31@gmail.com>
    Copyright (c) 2007-2008 by Michel Saliba      <msalibaba@gmail.com>

    Kopete    (c) 2002-2008 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef _UPNPKOPETE_H_
#define _UPNPKOPETE_H_


#include <stdio.h>
#include <string.h>

#include <upnp/ixml.h>
#include <upnp/FreeList.h>
#include <upnp/iasnprintf.h>
#include <upnp/ithread.h>
#include <upnp/LinkedList.h>
#include <upnp/ThreadPool.h>
#include <upnp/TimerThread.h>
#include <upnp/upnpconfig.h>
#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include <QList>
#include <QString>
#include <QChar>

#include "device.h"
#include "sample_util.h"

#include "util_Xml.h"

#define RESEARCH_SUCCESS 0
#define RESEARCH_ERROR 1
#define ACTION_SUCCESS 0
#define ACTION_ERROR 1

class UpnpKopete 
{
	public:
		~UpnpKopete();
		
		static UpnpKopete* getInstance();
		QString hostIp();
		unsigned short destPort();
		
		QString routeurLocation();
		QString descDoc();
		Device mainDevices();
		void viewDevice();

		void setRouteurLocation(QString routeurLocation);
		void setDescDoc(QString url);

		void addDevice(IXML_Document * DescDoc,QString location);
		//different actions
		int upnpConnect();
		int openPort(QString nameProtocol, int numPort);
		int deletePort(int numPort);
		int statusInfo();
		int portMapping();
	private:
		
		static UpnpKopete * m_uniqueInstance;		
		
		QString m_routeurLocation;
		QString m_deviceType;
		QString m_hostIp;
		unsigned short m_destPort;
		QString m_descDoc;
		UpnpClient_Handle m_device_handle;
		Device m_mainDevices;	

		UpnpKopete();
		
		IXML_Document * sendAction(QString nameAction, QList<QString> paramNameAction,QList<QString> paramValueAction);
};
#endif
/*
 * mediamanager.h - A manager for the media sessions.
 *
 * Copyright (c) 2008 by Detlev Casanova <detlev.casanova@gmail.com>
 *
 * Kopete    (c) by the Kopete developers  <kopete-devel@kde.org>
 *
 * *************************************************************************
 * *                                                                       *
 * * This program is free software; you can redistribute it and/or modify  *
 * * it under the terms of the GNU General Public License as published by  *
 * * the Free Software Foundation; either version 2 of the License, or     *
 * * (at your option) any later version.                                   *
 * *                                                                       *
 * *************************************************************************
 */
#ifndef MEDIA_MANAGER_H
#define MEDIA_MANAGER_H

#include <QObject>
#include <QByteArray>

class AlsaIO;
class MediaManager : public QObject
{
	Q_OBJECT
public:
	MediaManager(QString, QString);
	~MediaManager();
	AlsaIO *alsaIn() const;
	AlsaIO *alsaOut() const;

	bool start();

	QByteArray read();
	void write(const QByteArray& data);

private:
	class Private;
	Private *d;
};

#endif
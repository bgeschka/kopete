/*
 * jinglecrawontent.h - Jingle content
 * Copyright (C) 2008 - Detlev Casanova <detlev.casanova@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef JINGLE_RAW_CONTENT_H
#define JINGLE_RAW_CONTENT_H

#include <QObject>

#include "im.h"
#include "jinglecontent.h"

class QHostAddress;
class QDomElement;
class QUdpSocket;
namespace XMPP
{
	/*
	 * This class contains all information about a particular content in a jingle session.
	 * It also has the socket that will be used for streaming.
	 * This is a content for the Raw Udp transport method.
	 * TODO:use multiple candidates to establish multiple streams (RTP + RTCP).
	 */
	class JingleSession;
	class IRIS_EXPORT JingleRawContent : public JingleContent
	{
		Q_OBJECT
	public:
		JingleRawContent(Mode mode = Unknown, JingleSession *parent = 0, Task *rootTask = 0);
		virtual ~JingleRawContent();
		
		virtual void addCandidate(const QDomElement&);
		virtual void addTransportInfo(const QDomElement&);
		virtual void setSession(JingleSession *sess);
		virtual QString transportNS() const;
		virtual void writeDatagram(const QByteArray&);
		virtual QByteArray readAll();

//	signals:
	protected:
		virtual void sendCandidates();
		virtual void addRemoteCandidate(const QDomElement&);
	private:
		class Private;
		Private *d;
		void createUdpInSocket();
		void createUdpOutSocket(const QHostAddress& address, int port);
		void bind(const QHostAddress& address, int port);
		QDomElement findCandidate();
	};
}

#endif //JINGLE_RAW_CONTENT_H

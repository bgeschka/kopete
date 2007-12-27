/*
    statisticsdialog.cpp - Kopete Statistics Dialog

    Copyright (c) 2003-2004 by Marc Cramdal        <marc.cramdal@gmail.com>

    Copyright (c) 2007      by the Kopete Developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/


#include <qtabwidget.h>
#include <qwidget.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qstring.h>
#include <QTimeEdit>
#include <kvbox.h>

#include "kdialog.h"
#include "klocale.h"
#include "khtml_part.h"
#include "kstandarddirs.h"
#include "kdatepicker.h"

#include "kopetemetacontact.h"
#include "kopeteonlinestatus.h"

#include "statisticsdialog.h"
#include "statisticscontact.h"
#include "ui_statisticswidgetbase.h"
#include "statisticsplugin.h"
#include "statisticsdb.h"

StatisticsDialog::StatisticsDialog(StatisticsContact *contact, StatisticsDB *db, QWidget* parent) : KDialog(parent), m_db(db), m_contact(contact)
{
	setAttribute (Qt::WA_DeleteOnClose, true);
	setButtons (KDialog::Close);
	setDefaultButton(KDialog::Close);
	setCaption (i18n("Statistics for %1", contact->metaContact()->displayName()));
	QWidget * w = new QWidget (this);
	dialogUi = new Ui::StatisticsWidgetUI();
	dialogUi->setupUi(w);
	setMainWidget(w);
	

	KHBox *hbox = new KHBox(this);
	
	generalHTMLPart = new KHTMLPart(hbox);
	connect ( generalHTMLPart->browserExtension(), SIGNAL( openUrlRequestDelayed( const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments & ) ),
			  this, SLOT( slotOpenURLRequest( const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments & ) ) );
	generalHTMLPart->setJScriptEnabled(false);
	generalHTMLPart->setJavaEnabled(false);
	generalHTMLPart->setMetaRefreshEnabled(false);
	generalHTMLPart->setPluginsEnabled(false);
	generalHTMLPart->setOnlyLocalReferences(true);
	
	
	dialogUi->tabWidget->insertTab(0, hbox, i18n("General"));
	dialogUi->tabWidget->setCurrentIndex(0);
	
	dialogUi->timePicker->setTime(QTime::currentTime());
	dialogUi->datePicker->setDate(QDate::currentDate());
	connect(dialogUi->askButton, SIGNAL(clicked()), this, SLOT(slotAskButtonClicked()));
	
	setFocus();
	setEscapeButton(Close);
	
	generatePageGeneral();
	
	resize( 800, 600 );

}

StatisticsDialog::~StatisticsDialog()
{
	delete dialogUi;
}

// We only generate pages when the user clicks on a link
void StatisticsDialog::slotOpenURLRequest(const KUrl& url, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &)
{
	if (url.protocol() == "main")
	{
		generatePageGeneral();
	}
	else if (url.protocol() == "dayofweek")
	{
		generatePageForDay(url.path().toInt());
	}
	else if (url.protocol() == "monthofyear")
	{
		generatePageForMonth(url.path().toInt());
	}
}

/*void StatisticsDialog::parseTemplate(QString Template)
{
	QString fileString = ::locate("appdata", "kopete_statistics.template.html");
	QString templateString;
	QFile file(file);
	if (file.open(QIODevice::ReadOnly))
	{
		QTextStream stream(&file);
		templateString = stream.read();
		file.close();
	}	
	// The template is loaded in templateString now.
	templateString.strReplace(
}*/

void StatisticsDialog::generatePageForMonth(const int monthOfYear)
{
	QStringList values = m_db->query(QString("SELECT status, datetimebegin, datetimeend "
	"FROM contactstatus WHERE metacontactid LIKE '%1' ORDER BY datetimebegin;").arg(m_contact->metaContact()->metaContactId()));	
	
	QStringList values2;
	
	for (int i=0; i<values.count(); i+=3)
	{
		QDateTime dateTimeBegin;
		dateTimeBegin.setTime_t(values[i+1].toInt());
		/// @todo Same as for Day, check if second datetime is on the same month
		if (dateTimeBegin.date().month() == monthOfYear)
		{
			values2.push_back(values[i]);
			values2.push_back(values[i+1]);
			values2.push_back(values[i+2]);
		}
	}
	generatePageFromQStringList(values2, QDate::longMonthName(monthOfYear));
}

void StatisticsDialog::generatePageForDay(const int dayOfWeek)
{
	QStringList values = m_db->query(QString("SELECT status, datetimebegin, datetimeend "
	"FROM contactstatus WHERE metacontactid LIKE '%1' ORDER BY datetimebegin;").arg(m_contact->metaContact()->metaContactId()));	
	
	QStringList values2;
	
	for (int i=0; i<values.count(); i+=3)
	{
		QDateTime dateTimeBegin;
		dateTimeBegin.setTime_t(values[i+1].toInt());
		QDateTime dateTimeEnd;
		dateTimeEnd.setTime_t(values[i+2].toInt());
		if (dateTimeBegin.date().dayOfWeek() == dayOfWeek)
		{
			if (dateTimeEnd.date().dayOfWeek() != dayOfWeek)
		  // Day of week is not the same at beginning and at end of the event
		  {
		    values2.push_back(values[i]);
		    values2.push_back(values[i+1]);
			
			// datetime from value[i+1]
			
			dateTimeBegin = QDateTime(dateTimeBegin.date(), QTime(0, 0, 0));
			dateTimeBegin.addSecs(dateTimeBegin.time().secsTo(QTime(23, 59, 59)));
			values2.push_back(QString::number(dateTimeBegin.toTime_t()));
		  }
		  else
		    {
		      values2.push_back(values[i]);
		      values2.push_back(values[i+1]);
		      values2.push_back(values[i+2]);
		    }
		}
	}
	generatePageFromQStringList(values2, QDate::longDayName(dayOfWeek));
 
}

/// @todo chart problem at midnight.
void StatisticsDialog::generatePageFromQStringList(QStringList values, const QString & subTitle)
{
	generalHTMLPart->begin();
	generalHTMLPart->write(QString("<html><head><style>.bar { margin:0px;} "
	"body"
	"{"
	"font-size:11px"
	"}"
	".chart"								// Style for the charts
	"{ height:100px;"
	"border-left:1px solid #999;"
	"border-bottom:1px solid #999;"
	"vertical-align:bottom;"
	"}"
	".statgroup"							// Style for groups of similar statistics
	 "{ margin-bottom:10px;"
	 "background-color:white;"
	"border-left: 5px solid #369;"
	"border-top: 1px dashed #999;"
	"border-bottom: 1px dashed #999;"
	"margin-left: 10px;"
	"margin-right: 5px;"
	"padding:3px 3px 3px 10px;}"
	"</style></head><body>" + 
	i18n("<h1>Statistics for %1</h1>", m_contact->metaContact()->displayName()) +
	"<h3>%1</h3><hr>").arg(subTitle));
	
	generalHTMLPart->write(i18n("<div class=\"statgroup\"><b><a href=\"main:generalinfo\" title=\"General summary view\">General</a></b><br>"
	"<span title=\"Select the day or month for which you want to view statistics\"><b>Days: </b>"
	"<a href=\"dayofweek:1\">Monday</a>&nbsp;"
	"<a href=\"dayofweek:2\">Tuesday</a>&nbsp;"
	"<a href=\"dayofweek:3\">Wednesday</a>&nbsp;"
	"<a href=\"dayofweek:4\">Thursday</a>&nbsp;"
	"<a href=\"dayofweek:5\">Friday</a>&nbsp;"
	"<a href=\"dayofweek:6\">Saturday</a>&nbsp;"
	"<a href=\"dayofweek:7\">Sunday</a><br>"
	"<b>Months: </b>"
	"<a href=\"monthofyear:1\">January</a>&nbsp;"
	"<a href=\"monthofyear:2\">February</a>&nbsp;"
	"<a href=\"monthofyear:3\">March</a>&nbsp;"
	"<a href=\"monthofyear:4\">April</a>&nbsp;"
	"<a href=\"monthofyear:5\">May</a>&nbsp;"
	"<a href=\"monthofyear:6\">June</a>&nbsp;"
	"<a href=\"monthofyear:7\">July</a>&nbsp;"
	"<a href=\"monthofyear:8\">August</a>&nbsp;"
	"<a href=\"monthofyear:9\">September</a>&nbsp;"
	"<a href=\"monthofyear:10\">October</a>&nbsp;"
	"<a href=\"monthofyear:11\">November</a>&nbsp;"
	"<a href=\"monthofyear:12\">December</a>&nbsp;"
	"</span></div><br>"));
	
//	dialogUi->listView->addColumn(i18n("Status"));
//	dialogUi->listView->addColumn(i18n("Start Date"));
//	dialogUi->listView->addColumn(i18n("End Date"));
//	dialogUi->listView->addColumn(i18n("Start Date"));
//	dialogUi->listView->addColumn(i18n("End Date"));

	QString todayString;
	todayString.append(i18n("<div class=\"statgroup\" title=\"Contact status history for today\"><h2>Today</h2><table width=\"100%\"><tr><td>Status</td><td>From</td><td>To</td></tr>"));
	
	bool today;
	
	int totalTime = 0; // this is in seconds
	int totalAwayTime = 0; // this is in seconds
	int totalOnlineTime = 0; // this is in seconds
	int totalOfflineTime = 0; // idem
	
	int hours[24]; // in seconds, too
	int iMaxHours = 0;
	int hoursOnline[24]; // this is in seconds
	int iMaxHoursOnline = 0;
	int hoursAway[24]; // this is in seconds
	int iMaxHoursAway = 0;
	int hoursOffline[24]; // this is in seconds. Hours where we are sure contact is offline
	int iMaxHoursOffline = 0;
	
	for (uint i=0; i<24; i++)
	{
		hours[i] = 0;
		hoursOnline[i] = 0;
		hoursAway[i] = 0;
		hoursOffline[i] = 0;
	}
		
	for (int i=0; i<values.count(); i+=3 /* because SELECT 3 columns */)
	{
		/* 	Here we try to interpret one database entry...
			What's important here, is to not count two times the same hour for instance
			This is why there are some if in all this stuff ;-)
		*/
		
		
		// it is the STARTDATE from the database
		QDateTime dateTime1;
		dateTime1.setTime_t(values[i+1].toInt());
		// it is the ENDDATE from the database
		QDateTime dateTime2;
		dateTime2.setTime_t(values[i+2].toInt());
		
		if (dateTime1.date() == QDate::currentDate() || dateTime2.date() == QDate::currentDate())
			today = true;
		else today = false;
		
		totalTime += dateTime1.secsTo(dateTime2);
		
		if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Online) 
			totalOnlineTime += dateTime1.secsTo(dateTime2);
		else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Away) 
			totalAwayTime += dateTime1.secsTo(dateTime2);
		else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Offline) 
			totalOfflineTime += dateTime1.secsTo(dateTime2);
	
		
		/*
		 * To build the chart/hours
		 */
		 
		// Number of hours between dateTime1 and dateTime2
		int nbHours = (int)(dateTime1.secsTo(dateTime2)/3600.0);
		
		uint tempHour = 
			dateTime1.time().hour() == dateTime2.time().hour()
				 ? dateTime1.secsTo(dateTime2) // (*)
				: 3600 - dateTime1.time().minute()*60 - dateTime1.time().second();
		hours[dateTime1.time().hour()] += tempHour;
		
		if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Online) 			
			hoursOnline[dateTime1.time().hour()] += tempHour;
		else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Away) 	
			hoursAway[dateTime1.time().hour()] += tempHour;
		else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Offline) 
			hoursOffline[dateTime1.time().hour()] += tempHour;
		
		for (int j= dateTime1.time().hour()+1; j < dateTime1.time().hour() + nbHours - 1; j++)
		{
			hours[j%24] += 3600;
			if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Online) 
				hoursOnline[j%24] += 3600;
			else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Away) 
				hoursAway[j%24] += 3600;
			else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Offline) 	
				hoursOffline[j%24] += 3600;
		}
		
		
		if (dateTime1.time().hour() != dateTime2.time().hour())
		// We don't want to count this if the hour from dateTime2 is the same than the one from dateTime1
		// since it as already been taken in account in the (*) instruction
		{
			tempHour = dateTime2.time().minute()*60 +dateTime2.time().second();
			hours[dateTime2.time().hour()] += tempHour;
			
			if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Online) 
				hoursOnline[dateTime2.time().hour()] += tempHour;
			else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Away) 	
				hoursAway[dateTime2.time().hour()] += tempHour;
			else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Offline) 
				hoursOffline[dateTime2.time().hour()] += tempHour;
			
					
		}
		
		
		
		QString color;
		if (today)
		{
			QString status;
			if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Online) 
			{
				color="blue";
				status = i18n("Online");
			}
			else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Away) 	
			{
				color="navy";
				status = i18n("Away");
			}
			else if (Kopete::OnlineStatus::statusStringToType(values[i]) == Kopete::OnlineStatus::Offline) 
			{
				color="gray";
				status = i18n("Offline");
			}
			else color="white";
		
			todayString.append(QString("<tr style=\"color:%1\"><td>%2</td><td>%3</td><td>%4</td></tr>").arg(color, status, dateTime1.time().toString(), dateTime2.time().toString()));
		
		}
		
		// We add a listview item to the log list
		// QDateTime listViewDT1, listViewDT2;
		// listViewDT1.setTime_t(values[i+1].toInt());
		// listViewDT2.setTime_t(values[i+2].toInt());
		// new K3ListViewItem(dialogUi->listView, values[i], values[i+1], values[i+2], listViewDT1.toString(), listViewDT2.toString());
	}

	
	todayString.append("</table></div>");
	
	// Get the max from the hours*
	for (uint i=1; i<24; i++)
	{
		if (hours[iMaxHours] < hours[i])
			iMaxHours = i;
		if (hoursOnline[iMaxHoursOnline] < hoursOnline[i])
			iMaxHoursOnline = i;
		if (hoursOffline[iMaxHoursOffline] < hoursOffline[i])
			iMaxHoursOffline = i;
		if (hoursAway[iMaxHoursAway] < hoursAway[i])
			iMaxHoursAway = i;	
	}
	
	//
	
	/*
	 * Here we really generate the page
	 */
	// Some "total times"
	generalHTMLPart->write(i18n("<div class=\"statgroup\">"));
	generalHTMLPart->write(i18n("<b title=\"The total time I have been able to see %1 status\">"
		"Total visible time :</b> %2 hour(s)<br>", m_contact->metaContact()->displayName(), stringFromSeconds(totalTime)));
	generalHTMLPart->write(i18n("<b title=\"The total time I have seen %1 online\">"
		"Total online time :</b> %2 hour(s)<br>", m_contact->metaContact()->displayName(), stringFromSeconds(totalOnlineTime)));
	generalHTMLPart->write(i18n("<b title=\"The total time I have seen %1 away\">Total busy time :</b> %2 hour(s)<br>", m_contact->metaContact()->displayName(), stringFromSeconds(totalAwayTime)));
	generalHTMLPart->write(i18n("<b title=\"The total time I have seen %1 offline\">Total offline time :</b> %2 hour(s)", m_contact->metaContact()->displayName(), stringFromSeconds(totalOfflineTime)));
	generalHTMLPart->write(QString("</div>"));

	if (subTitle == i18n("General information"))
	/*
	 * General stats that should not be shown on "day" or "month" pages
	 */
	{
		generalHTMLPart->write(QString("<div class=\"statgroup\">"));
		generalHTMLPart->write(i18n("<b>Average message length :</b> %1 characters<br>", m_contact->messageLength()));
		generalHTMLPart->write(i18n("<b>Time between two messages : </b> %1 second(s)", m_contact->timeBetweenTwoMessages()));
		generalHTMLPart->write(QString("</div>"));
	
		generalHTMLPart->write(QString("<div class=\"statgroup\">"));
		generalHTMLPart->write(i18n("<b title=\"The last time you talked with %1\">Last talk :</b> %2<br>", m_contact->metaContact()->displayName(), KGlobal::locale()->formatDateTime(m_contact->lastTalk())));
		generalHTMLPart->write(i18n("<b title=\"The last time I have seen %1 online or away\">Last time present :</b> %2", m_contact->metaContact()->displayName(), KGlobal::locale()->formatDateTime(m_contact->lastPresent())));
		generalHTMLPart->write(QString("</div>"));

		//generalHTMLPart->write(QString("<div class=\"statgroup\">"));
		//generalHTMLPart->write(i18n("<b title=\"%1 uses to set his status online at these hours (EXPERIMENTAL)\">Main online events :</b><br>").arg(m_contact->metaContact()->displayName()));
		//QValueList<QTime> mainEvents = m_contact->mainEvents(Kopete::OnlineStatus::Online);
		//for (uint i=0; i<mainEvents.count(); i++)
		//generalHTMLPart->write(QString("%1<br>").arg(mainEvents[i].toString()));
		//generalHTMLPart->write(QString("</div>"));
 
		generalHTMLPart->write("<div title=\"" +i18n("Current status") + "\" class=\"statgroup\">");
		generalHTMLPart->write(i18n("Is <b>%1</b> since <b>%2</b>",
				Kopete::OnlineStatus(m_contact->oldStatus()).description(),
				KGlobal::locale()->formatDateTime(m_contact->oldStatusDateTime())));
		generalHTMLPart->write(QString("</div>"));
	}
		
	/*
	 * Chart which show the hours where plugin has seen this contact online
	 */
	generalHTMLPart->write(QString("<div class=\"statgroup\">"));
	generalHTMLPart->write(QString("<table width=\"100%\"><tr><td colspan=\"3\">") + i18n("When have I seen this contact ?") + QString("</td></tr>"));
	generalHTMLPart->write(QString("<tr><td height=\"200\" valign=\"bottom\" colspan=\"3\" class=\"chart\">"));
	
	QString chartString;
	QString colorPath = KStandardDirs::locate("appdata", "pics/statistics/black.png");
	for (uint i=0; i<24; i++)
	{
		
		int hrWidth = qRound((double)hours[i]/(double)hours[iMaxHours]*100.);
		chartString += QString("<img class=\"margin:0px;\"  height=\"")
				+(totalTime ? QString::number(hrWidth) : QString::number(0))
				+QString("\" src=\"file://")
				+colorPath
				+"\" width=\"4%\" title=\""
				+i18n("Between %1:00 and %2:00, I was able to see %3 status %4% of the hour.", i, (i+1)%24, m_contact->metaContact()->displayName(), hrWidth)
				+QString("\">");
	}
	generalHTMLPart->write(chartString);
	generalHTMLPart->write(QString("</td></tr>"));
	
	
	
	generalHTMLPart->write(QString(	"<tr>"
		"<td>")+i18n("Online time")+QString("</td><td>")+i18n("Away time")+QString("</td><td>")+i18n("Offline time")+QString("</td>"
					"</tr>"
					"<td valign=\"bottom\" width=\"33%\" class=\"chart\">"));
	
	
	generalHTMLPart->write(generateHTMLChart(hoursOnline, hoursAway, hoursOffline, i18n("online"), "blue"));
	generalHTMLPart->write(QString("</td><td valign=\"bottom\" width=\"33%\" class=\"chart\">"));
	generalHTMLPart->write(generateHTMLChart(hoursAway, hoursOnline, hoursOffline, i18n("away"), "navy"));
	generalHTMLPart->write(QString("</td><td valign=\"bottom\" width=\"33%\" class=\"chart\">"));
	generalHTMLPart->write(generateHTMLChart(hoursOffline, hoursAway, hoursOnline, i18n("offline"), "gray"));
	generalHTMLPart->write(QString("</td></tr></table></div>"));
		
	if (subTitle == i18n("General information"))
	/* On main page, show the different status of the contact today
	 */
	{
		generalHTMLPart->write(QString(todayString));
	}
	generalHTMLPart->write(QString("</body></html>"));
	
	generalHTMLPart->end();
	
}

void StatisticsDialog::generatePageGeneral()
{
	QStringList values;
	values = m_db->query(QString("SELECT status, datetimebegin, datetimeend "
				"FROM contactstatus WHERE metacontactid LIKE '%1' ORDER BY datetimebegin;")
				.arg(m_contact->metaContact()->metaContactId()));
	generatePageFromQStringList(values, i18n("General information"));
}

QString StatisticsDialog::generateHTMLChart(const int *hours, const int *hours2, const int *hours3, const QString & caption, const QString & color)
{
	QString chartString;
	
	QString colorPath = KStandardDirs::locate("appdata", "pics/statistics/"+color+".png");
	
	
	for (uint i=0; i<24; i++)
	{
		int totalTime = hours[i] + hours2[i] + hours3[i];
		
		int hrWidth = qRound((double)hours[i]/(double)totalTime*100.);
		chartString += QString("<img class=\"margin:0px;\"  height=\"")
				+(totalTime ? QString::number(hrWidth) : QString::number(0))
				+QString("\" src=\"file://")
				+colorPath
				+"\" width=\"4%\" title=\""+
				i18n("Between %1:00 and %2:00, I have seen %3 %4% %5.", 
				i, 
				(i+1) % 24, 
				m_contact->metaContact()->displayName(), 
				hrWidth, 
				caption)
				+".\">";
	}
	return chartString;
}

QString StatisticsDialog::stringFromSeconds(const int seconds)
{
	int h, m, s;
	h = seconds/3600;
	m = (seconds % 3600)/60;
	s = (seconds % 3600) % 60;
	return QString::number(h)+':'+QString::number(m)+':'+QString::number(s);
}

void StatisticsDialog::slotAskButtonClicked()
{
	if (dialogUi->questionComboBox->currentIndex()==0)
	{
		QString text = i18nc("1 is date, 2 is contact name, 3 is online status", "%1, %2 was %3",
		     KGlobal::locale()->formatDateTime(QDateTime(dialogUi->datePicker->date(), dialogUi->timePicker->time())),
		     m_contact->metaContact()->displayName(),
		     m_contact->statusAt(QDateTime(dialogUi->datePicker->date(), dialogUi->timePicker->time())));
		dialogUi->answerEdit->setText(text);
	}
	else if (dialogUi->questionComboBox->currentIndex()==1)
	{
		dialogUi->answerEdit->setText(m_contact->mainStatusDate(dialogUi->datePicker->date()));
	}
	else if (dialogUi->questionComboBox->currentIndex()==2)
	// Next online
	{
		
	}
}

#include "statisticsdialog.moc"
/*
    statisticsdb.cpp

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

#include <qfile.h>
//Added by qt3to4:
#include <QByteArray>

#include "sqlite3.h"

#include <kgenericfactory.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kdeversion.h>

#include "statisticsdb.h"

#include <unistd.h>
#include <time.h>

StatisticsDB::StatisticsDB()
{
	QByteArray path = (KStandardDirs::locateLocal("appdata", "kopete_statistics-0.1.db")).toLatin1();
	kDebug(14315) << "statistics: DB path:" << path;

	// Open database file and check for correctness
	bool failOpen = true;
	QFile file( path );
	if ( file.open( QIODevice::ReadOnly ) ) 
	{
		QString format;
		format = QString( file.readLine( 50 ) ); //  readLine return a QByteArray
		if ( !format.startsWith( "SQLite format 3" ) ) 
		{
			kWarning() << "[statistics] Database versions incompatible. Removing and rebuilding database.\n";
		}
		else if ( sqlite3_open( path, &m_db ) != SQLITE_OK ) 
		{
			kWarning() << "[statistics] Database file corrupt. Removing and rebuilding database.\n";
			sqlite3_close( m_db );
		}
		else
			failOpen = false;
	}
	
	if ( failOpen ) 
	{
	// Remove old db file; create new
	QFile::remove( path );
	sqlite3_open( path, &m_db );
	}

	kDebug(14315) << "[Statistics] Contructor";

	// Creates the tables if they do not exist.
	QStringList result = query("SELECT name FROM sqlite_master WHERE type='table'");
	
	if (!result.contains("contactstatus"))
	{
		kDebug(14315) << "[Statistics] Database empty";
		query(QString("CREATE TABLE contactstatus "
			"(id INTEGER PRIMARY KEY,"
			"metacontactid TEXT,"
			"status TEXT,"
			"datetimebegin INTEGER,"
			"datetimeend INTEGER"
			");"));
	}
	
	if (!result.contains("commonstats"))
	{
		// To store things like the contact answer time etc.
		query(QString("CREATE TABLE commonstats"
			" (id INTEGER PRIMARY KEY,"
			"metacontactid TEXT,"
			"statname TEXT," // for instance, answertime, lastmessage, messagelength ...
			"statvalue1 TEXT,"
			"statvalue2 TEXT"
			");"));
	}

	if (!result.contains("statsgroup"))
	{
		query(QString("CREATE TABLE statsgroup"
			"(id INTEGER PRIMARY KEY,"
			"datetimebegin INTEGER,"
			"datetimeend INTEGER,"
			"caption TEXT);"));
	}
 
}

StatisticsDB::~StatisticsDB()
{
	sqlite3_close(m_db);
} 

 /**
  * Executes a SQL query on the already opened database
  * @param statement SQL program to execute. Only one SQL statement is allowed.
  * @param debug     Set to true for verbose debug output.
  * @retval names    Will contain all column names, set to NULL if not used.
  * @return          The queried data, or QStringList() on error.
  */
 QStringList StatisticsDB::query( const QString& statement, QStringList* const names, bool debug )
 {
 
     if ( debug )
	     kDebug(14315) << "query-start: " << statement;
 
     clock_t start = clock();
 
     if ( !m_db )
     {
         kError(14315) << "[CollectionDB] SQLite pointer == NULL.\n";
         return QStringList();
     }
 
     int error;
     QStringList values;
     const char* tail;
     sqlite3_stmt* stmt;
 
     //compile SQL program to virtual machine
     error = sqlite3_prepare( m_db, statement.toUtf8(), statement.length(), &stmt, &tail );
 
     if ( error != SQLITE_OK )
     {
         kError(14315) << "[CollectionDB] sqlite3_compile error:" << endl;
         kError(14315) << sqlite3_errmsg( m_db ) << endl;
         kError(14315) << "on query: " << statement << endl;
 
         return QStringList();
     }
 
     int busyCnt = 0;
     int number = sqlite3_column_count( stmt );
     //execute virtual machine by iterating over rows
     while ( true )
     {
         error = sqlite3_step( stmt );
 
         if ( error == SQLITE_BUSY )
         {
             if ( busyCnt++ > 20 ) {
                 kError(14315) << "[CollectionDB] Busy-counter has reached maximum. Aborting this sql statement!\n";
                 break;
             }
             ::usleep( 100000 ); // Sleep 100 msec
	     kDebug(14315) << "[CollectionDB] sqlite3_step: BUSY counter: " << busyCnt;
         }
         if ( error == SQLITE_MISUSE )
		 kDebug(14315) << "[CollectionDB] sqlite3_step: MISUSE";
         if ( error == SQLITE_DONE || error == SQLITE_ERROR )
             break;
 
         //iterate over columns
         for ( int i = 0; i < number; i++ )
         {
             values << QString::fromUtf8( (const char*) sqlite3_column_text( stmt, i ) );
             if ( names ) *names << QString( sqlite3_column_name( stmt, i ) );
         }
     }
     //deallocate vm resources
     sqlite3_finalize( stmt );
 
     if ( error != SQLITE_DONE )
     {
         kError(14315) << "sqlite_step error.\n";
         kError(14315) << sqlite3_errmsg( m_db ) << endl;
         kError(14315) << "on query: " << statement << endl;
 
         return QStringList();
     }
 
     if ( debug )
     {
         clock_t finish = clock();
         const double duration = (double) (finish - start) / CLOCKS_PER_SEC;
	 kDebug(14315) << "[CollectionDB] SQL-query (" << duration << "s): " << statement;
     }
 
 
    return values;
}
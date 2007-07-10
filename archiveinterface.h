/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ARCHIVEINTERFACE_H
#define ARCHIVEINTERFACE_H

#include <QObject>
#include <QStringList>
#include <QString>

#include "arch.h"

class ArchiveObserver
{
	public:
		ArchiveObserver() {}
		virtual ~ArchiveObserver() {}

		virtual void onError( const QString & message, const QString & details ) = 0;
		virtual void onEntry( const ArchiveEntry & archiveEntry ) = 0;
		virtual void onProgress( double ) = 0;
};

class ReadOnlyArchiveInterface: public QObject
{
	Q_OBJECT
	public:
		ReadOnlyArchiveInterface( const QString & filename, QObject *parent = 0 );
		virtual ~ReadOnlyArchiveInterface();

		QString filename() const { return m_filename; }
		virtual bool isReadOnly() const { return true; }

		void registerObserver( ArchiveObserver *observer );
		void removeObserver( ArchiveObserver *observer );

		virtual bool open() { return true; }
		virtual bool list() = 0;
		virtual bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory ) = 0;

	protected:
		void error( const QString & message, const QString & details );
		void entry( const ArchiveEntry & archiveEntry );
		void progress( double );

	private:
		QList<ArchiveObserver*> m_observers;
		QString m_filename;
};
/*
class ReadWriteArchiveInterface: public ReadOnlyArchiveInterface
{
	Q_OBJECT
};
*/
#endif // ARCHIVEINTERFACE_H

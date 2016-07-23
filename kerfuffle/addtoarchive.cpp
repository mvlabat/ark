/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "addtoarchive.h"
#include "ark_debug.h"
#include "archive_kerfuffle.h"
#include "createdialog.h"
#include "jobs.h"

#include <KConfig>
#include <kjobtrackerinterface.h>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <kio/job.h>

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QMimeDatabase>
#include <QTimer>
#include <QPointer>

namespace Kerfuffle
{
AddToArchive::AddToArchive(QObject *parent)
        : KJob(parent), m_changeToFirstPath(false)
{
}

AddToArchive::~AddToArchive()
{
}

void AddToArchive::setAutoFilenameSuffix(const QString& suffix)
{
    m_autoFilenameSuffix = suffix;
}

void AddToArchive::setChangeToFirstPath(bool value)
{
    m_changeToFirstPath = value;
}

void AddToArchive::setFilename(const QUrl &path)
{
    m_filename = path.toDisplayString(QUrl::PreferLocalFile);
}

void AddToArchive::setMimeType(const QString & mimeType)
{
    m_mimeType = mimeType;
}

void AddToArchive::setPassword(const QString &password)
{
    m_password = password;
}

void AddToArchive::setHeaderEncryptionEnabled(bool enabled)
{
    m_enableHeaderEncryption = enabled;
}

bool AddToArchive::showAddDialog()
{
    qCDebug(ARK) << "Opening add dialog";

    QPointer<Kerfuffle::CreateDialog> dialog = new Kerfuffle::CreateDialog(
        Q_NULLPTR, // parent
        i18n("Compress to Archive"), // caption
        QUrl::fromLocalFile(m_firstPath)); // startDir

    bool ret = dialog.data()->exec();

    if (ret) {
        qCDebug(ARK) << "CreateDialog returned URL:" << dialog.data()->selectedUrl().toString();
        qCDebug(ARK) << "CreateDialog returned mime:" << dialog.data()->currentMimeType().name();
        setFilename(dialog.data()->selectedUrl());
        setMimeType(dialog.data()->currentMimeType().name());
        setPassword(dialog.data()->password());
        setHeaderEncryptionEnabled(dialog.data()->isHeaderEncryptionEnabled());
    }

    delete dialog.data();

    return ret;
}

bool AddToArchive::addInput(const QUrl &url)
{
    Archive::Entry *entry = new Archive::Entry();
    entry->setFullPath(url.toDisplayString(QUrl::PreferLocalFile));
    m_entries << entry;

    if (m_firstPath.isEmpty()) {
        QString firstEntry = url.toDisplayString(QUrl::PreferLocalFile);
        m_firstPath = QFileInfo(firstEntry).dir().absolutePath();
    }

    return true;
}

void AddToArchive::start()
{
    qCDebug(ARK) << "Starting job";

    QTimer::singleShot(0, this, &AddToArchive::slotStartJob);
}

void AddToArchive::slotStartJob()
{
    Kerfuffle::CompressionOptions options;

    if (m_entries.isEmpty()) {
        KMessageBox::error(NULL, i18n("No input files were given."));
        emitResult();
        return;
    }

    Kerfuffle::Archive *archive;
    if (!m_filename.isEmpty()) {
        archive = Kerfuffle::Archive::create(m_filename, m_mimeType, this);
        qCDebug(ARK) << "Set filename to " << m_filename;
    } else {
        if (m_autoFilenameSuffix.isEmpty()) {
            KMessageBox::error(Q_NULLPTR, xi18n("You need to either supply a filename for the archive or a suffix (such as rar, tar.gz) with the <command>--autofilename</command> argument."));
            emitResult();
            return;
        }

        if (m_firstPath.isEmpty()) {
            qCWarning(ARK) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        const QString base = detectBaseName(m_entries);

        QString finalName = base + QLatin1Char( '.' ) + m_autoFilenameSuffix;

        //if file already exists, append a number to the base until it doesn't
        //exist
        int appendNumber = 0;
        while (QFileInfo::exists(finalName)) {
            ++appendNumber;
            finalName = base + QLatin1Char( '_' ) + QString::number(appendNumber) + QLatin1Char( '.' ) + m_autoFilenameSuffix;
        }

        qCDebug(ARK) << "Autoset filename to "<< finalName;
        archive = Kerfuffle::Archive::create(finalName, m_mimeType, this);
    }

    Q_ASSERT(archive);

    if (!archive->isValid()) {
        if (archive->error() == NoPlugin) {
            KMessageBox::error(Q_NULLPTR, i18n("Failed to create the new archive. No suitable plugin found."));
            emitResult();
            return;
        }
        if (archive->error() == FailedPlugin) {
            KMessageBox::error(Q_NULLPTR, i18n("Failed to create the new archive. Could not load a suitable plugin."));
            emitResult();
            return;
        }
    } else if (archive->isReadOnly()) {
        KMessageBox::error(Q_NULLPTR, i18n("It is not possible to create archives of this type."));
        emitResult();
        return;
    }

    if (!m_password.isEmpty()) {
        archive->encrypt(m_password, m_enableHeaderEncryption);
    }

    if (m_changeToFirstPath) {
        if (m_firstPath.isEmpty()) {
            qCWarning(ARK) << "Weird, this should not happen. no firstpath defined. aborting";
            emitResult();
            return;
        }

        const QDir stripDir(m_firstPath);

        foreach (Archive::Entry *entry, m_entries) {
            entry->setFullPath(stripDir.absoluteFilePath(entry->property("fullPath").toString()));
        }

        options[QStringLiteral( "GlobalWorkDir" )] = stripDir.path();
        qCDebug(ARK) << "Setting GlobalWorkDir to " << stripDir.path();
    }

    Kerfuffle::AddJob *job =
        archive->addFiles(m_entries, new Archive::Entry(this), options);

    KIO::getJobTracker()->registerJob(job);

    connect(job, &Kerfuffle::AddJob::result, this, &AddToArchive::slotFinished);

    job->start();
}

void AddToArchive::slotFinished(KJob *job)
{
    qCDebug(ARK) << "AddToArchive job finished";

    if (job->error() && !job->errorText().isEmpty()) {
        KMessageBox::error(Q_NULLPTR, job->errorText());
    }

    emitResult();
}

QString AddToArchive::detectBaseName(const QList<Archive::Entry*> &entries) const
{
    QFileInfo fileInfo = QFileInfo(entries.first()->property("fullPath").toString());
    QDir parentDir = fileInfo.dir();
    QString base = parentDir.absolutePath() + QLatin1Char('/');

    if (entries.size() > 1) {
        if (!parentDir.isRoot()) {
            // Use directory name for the new archive.
            base += parentDir.dirName();
        }
    } else {
        // Strip filename of its extension, but only if present (see #362690).
        if (!QMimeDatabase().mimeTypeForFile(fileInfo.fileName(), QMimeDatabase::MatchExtension).isDefault()) {
            base += fileInfo.completeBaseName();
        } else {
            base += fileInfo.fileName();
        }
    }

    // Special case for compressed tar archives.
    if (base.right(4).toUpper() == QLatin1String(".TAR")) {
        base.chop(4);
    }

    if (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }

    return base;
}

}

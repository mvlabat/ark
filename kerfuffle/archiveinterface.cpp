/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#include "archiveinterface.h"
#include "ark_debug.h"

#include <kfileitem.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace Kerfuffle
{
ReadOnlyArchiveInterface::ReadOnlyArchiveInterface(QObject *parent, const QVariantList & args)
        : QObject(parent)
        , m_waitForFinishedSignal(false)
        , m_isHeaderEncryptionEnabled(false)
        , m_isCorrupt(false)
{
    qCDebug(ARK) << "Created read-only interface for" << args.first().toString();
    m_filename = args.first().toString();
}

ReadOnlyArchiveInterface::~ReadOnlyArchiveInterface()
{
}

QString ReadOnlyArchiveInterface::filename() const
{
    return m_filename;
}

QString ReadOnlyArchiveInterface::comment() const
{
    return m_comment;
}

bool ReadOnlyArchiveInterface::isReadOnly() const
{
    return true;
}

bool ReadOnlyArchiveInterface::open()
{
    return true;
}

void ReadOnlyArchiveInterface::setPassword(const QString &password)
{
    m_password = password;
}

void ReadOnlyArchiveInterface::setHeaderEncryptionEnabled(bool enabled)
{
    m_isHeaderEncryptionEnabled = enabled;
}

QString ReadOnlyArchiveInterface::password() const
{
    return m_password;
}

bool ReadOnlyArchiveInterface::doKill()
{
    //default implementation
    return false;
}

bool ReadOnlyArchiveInterface::doSuspend()
{
    //default implementation
    return false;
}

bool ReadOnlyArchiveInterface::doResume()
{
    //default implementation
    return false;
}

void ReadOnlyArchiveInterface::setCorrupt(bool isCorrupt)
{
    m_isCorrupt = isCorrupt;
}

bool ReadOnlyArchiveInterface::isCorrupt() const
{
    return m_isCorrupt;
}

ReadWriteArchiveInterface::ReadWriteArchiveInterface(QObject *parent, const QVariantList & args)
        : ReadOnlyArchiveInterface(parent, args)
{
    qCDebug(ARK) << "Created read-write interface for" << args.first().toString();
}

ReadWriteArchiveInterface::~ReadWriteArchiveInterface()
{
}

bool ReadOnlyArchiveInterface::waitForFinishedSignal()
{
    return m_waitForFinishedSignal;
}

int ReadOnlyArchiveInterface::moveRequiredSignals() const {
    return 1;
}

int ReadOnlyArchiveInterface::copyRequiredSignals() const
{
    return 1;
}

void ReadOnlyArchiveInterface::setWaitForFinishedSignal(bool value)
{
    m_waitForFinishedSignal = value;
}

QStringList ReadOnlyArchiveInterface::entryFullPaths(const QList<Archive::Entry*> &entries, const bool withoutTrailingSlashes)
{
    QStringList filesList;
    foreach (const Archive::Entry *file, entries) {
        filesList << file->fullPath(withoutTrailingSlashes);
    }
    return filesList;
}

QList<Archive::Entry*> ReadOnlyArchiveInterface::entriesWithoutChildren(const QList<Archive::Entry*> &entries)
{
    // QMap is easy way to get entries sorted by their fullPath.
    QMap<QString, Archive::Entry*> sortedEntries;
    foreach (Archive::Entry *entry, entries) {
        sortedEntries.insert(entry->fullPath(), entry);
    }

    QList<Archive::Entry*> filteredEntries;
    QString lastFolder;
    foreach (Archive::Entry *entry, sortedEntries) {
        if (lastFolder.count() > 0 && entry->fullPath().startsWith(lastFolder))
            continue;

        lastFolder = (entry->fullPath().right(1) == QLatin1String("/")) ? entry->fullPath() : QString();
        filteredEntries << entry;
    }

    return filteredEntries;
}

QStringList ReadOnlyArchiveInterface::entryPathsFromDestination(QStringList entries, const Archive::Entry *destination, int entriesWithoutChildren)
{
    QStringList paths = QStringList();
    entries.sort();
    QString lastFolder;
    const QString destinationPath = (destination == Q_NULLPTR) ? QString() : destination->fullPath();

    QString newPath;
    int nameLength = 0;
        foreach (const QString &entryPath, entries) {
            if (lastFolder.count() > 0 && entryPath.startsWith(lastFolder)) {
                // Replace last moved or copied folder path with destination path.
                int charsCount = entryPath.count() - lastFolder.count();
                if (entriesWithoutChildren != 1) {
                    charsCount += nameLength;
                }
                newPath = destinationPath + entryPath.right(charsCount);
            }
            else {
                const QString name = entryPath.split(QLatin1Char('/'), QString::SkipEmptyParts).last();
                if (entriesWithoutChildren != 1) {
                    newPath = destinationPath + name;
                    if (entryPath.right(1) == QLatin1String("/")) {
                        newPath += QLatin1Char('/');
                    }
                }
                else {
                    // If the mode is set to Move and there is only one passed file in the list,
                    // we have to use destination as newPath.
                    newPath = destinationPath;
                }
                if (entryPath.right(1) == QLatin1String("/")) {
                    nameLength = name.count() + 1; // plus slash
                    lastFolder = entryPath;
                }
                else {
                    nameLength = 0;
                    lastFolder = QString();
                }
            }
            paths << newPath;
        }

    return paths;
}

bool ReadOnlyArchiveInterface::isHeaderEncryptionEnabled() const
{
    return m_isHeaderEncryptionEnabled;
}

bool ReadWriteArchiveInterface::isReadOnly() const
{
    // We set corrupt archives to read-only to avoid add/delete actions, that
    // are likely to fail anyway.
    if (isCorrupt()) {
        return true;
    }

    QFileInfo fileInfo(filename());
    if (fileInfo.exists()) {
        return !fileInfo.isWritable();
    } else {
        return !fileInfo.dir().exists(); // TODO: Should also check if we can create a file in that directory
    }
}

} // namespace Kerfuffle

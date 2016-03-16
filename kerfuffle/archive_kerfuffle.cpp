/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "archive_kerfuffle.h"
#include "ark_debug.h"
#include "archiveinterface.h"
#include "jobs.h"
#include "mimetypes.h"

#include <QByteArray>
#include <QDebug>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>

#include <KPluginFactory>
#include <KPluginLoader>

namespace Kerfuffle
{

bool Archive::comparePlugins(const KPluginMetaData &p1, const KPluginMetaData &p2)
{
    return (p1.rawData()[QStringLiteral("X-KDE-Priority")].toVariant().toInt()) > (p2.rawData()[QStringLiteral("X-KDE-Priority")].toVariant().toInt());
}

QVector<KPluginMetaData> Archive::findPluginOffers(const QString& filename, const QString& fixedMimeType)
{
    qCDebug(ARK) << "Find plugin offers for" << filename << "with mime" << fixedMimeType;

    const QString mimeType = fixedMimeType.isEmpty() ? determineMimeType(filename).name() : fixedMimeType;

    qCDebug(ARK) << "Detected mime" << mimeType;

    QVector<KPluginMetaData> offers = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"), [mimeType](const KPluginMetaData& metaData) {
        return metaData.serviceTypes().contains(QStringLiteral("Kerfuffle/Plugin")) &&
               metaData.mimeTypes().contains(mimeType);
    });

    qSort(offers.begin(), offers.end(), comparePlugins);
    qCDebug(ARK) << "Have" << offers.size() << "offers";

    return offers;
}

QDebug operator<<(QDebug d, const fileRootNodePair &pair)
{
    d.nospace() << "fileRootNodePair(" << pair.file << "," << pair.rootNode << ")";
    return d.space();
}

Archive *Archive::create(const QString &fileName, QObject *parent)
{
    return create(fileName, QString(), parent);
}

Archive *Archive::create(const QString &fileName, const QString &fixedMimeType, QObject *parent)
{
    qCDebug(ARK) << "Going to create archive" << fileName;

    qRegisterMetaType<ArchiveEntry>("ArchiveEntry");

    const QVector<KPluginMetaData> offers = findPluginOffers(fileName, fixedMimeType);
    if (offers.isEmpty()) {
        qCCritical(ARK) << "Could not find a plugin to handle" << fileName;
        return new Archive(NoPlugin, parent);
    }

    Archive *archive;
    foreach (const KPluginMetaData& pluginMetadata, offers) {
        archive = create(fileName, pluginMetadata, parent);
        // Use the first valid plugin, according to the priority sorting.
        if (archive->isValid()) {
            return archive;
        }
    }

    qCCritical(ARK) << "Failed to find a usable plugin for" << fileName;
    return archive;
}

Archive *Archive::create(const QString &fileName, const KPluginMetaData &pluginMetadata, QObject *parent)
{
    const bool isReadOnly = !pluginMetadata.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWrite")].toVariant().toBool();
    qCDebug(ARK) << "Loading plugin" << pluginMetadata.pluginId();

    KPluginFactory *factory = KPluginLoader(pluginMetadata.fileName()).factory();
    if (!factory) {
        qCWarning(ARK) << "Invalid plugin factory for" << pluginMetadata.pluginId();
        return new Archive(FailedPlugin, parent);
    }

    const QVariantList args = {QVariant(QFileInfo(fileName).absoluteFilePath())};
    ReadOnlyArchiveInterface *iface = factory->create<ReadOnlyArchiveInterface>(Q_NULLPTR, args);
    if (!iface) {
        qCWarning(ARK) << "Could not create plugin instance" << pluginMetadata.pluginId();
        return new Archive(FailedPlugin, parent);
    }

    // Not CliBased plugin, don't search for executables.
    if (!iface->isCliBased()) {
        return new Archive(iface, isReadOnly, parent);
    }

    qCDebug(ARK) << "Finding executables for plugin" << pluginMetadata.pluginId();

    if (iface->findExecutables(!isReadOnly)) {
        return new Archive(iface, isReadOnly, parent);
    }

    if (!isReadOnly && iface->findExecutables(false)) {
        qCWarning(ARK) << "Failed to find read-write executables: falling back to read-only mode for read-write plugin" << pluginMetadata.pluginId();
        return new Archive(iface, true, parent);
    }

    qCWarning(ARK) << "Failed to find needed executables for plugin" << pluginMetadata.pluginId();
    return new Archive(FailedPlugin, parent);
}

Archive::Archive(ArchiveError errorCode, QObject *parent)
        : QObject(parent)
        , m_error(errorCode)
{
    qCDebug(ARK) << "Created archive instance with error";
}

Archive::Archive(ReadOnlyArchiveInterface *archiveInterface, bool isReadOnly, QObject *parent)
        : QObject(parent)
        , m_iface(archiveInterface)
        , m_hasBeenListed(false)
        , m_isReadOnly(isReadOnly)
        , m_isSingleFolderArchive(false)
        , m_extractedFilesSize(0)
        , m_error(NoError)
        , m_encryptionType(Unencrypted)
        , m_numberOfFiles(0)
{
    qCDebug(ARK) << "Created archive instance";

    Q_ASSERT(archiveInterface);
    archiveInterface->setParent(this);

    QMetaType::registerComparators<fileRootNodePair>();
    QMetaType::registerDebugStreamOperator<fileRootNodePair>();

    connect(m_iface, &ReadOnlyArchiveInterface::entry, this, &Archive::onNewEntry);
}


Archive::~Archive()
{
}

QString Archive::completeBaseName() const
{
    QString base = QFileInfo(fileName()).completeBaseName();

    // Special case for compressed tar archives.
    if (base.right(4).toUpper() == QLatin1String(".TAR")) {
        base.chop(4);
    }

    return base;
}

QString Archive::fileName() const
{
    return m_iface->filename();
}

QString Archive::comment() const
{
    return m_iface->comment();
}

QMimeType Archive::mimeType() const
{
    return determineMimeType(fileName());
}

bool Archive::isReadOnly() const
{
    return (m_iface->isReadOnly() || m_isReadOnly);
}

bool Archive::isSingleFolderArchive()
{
    listIfNotListed();
    return m_isSingleFolderArchive;
}

bool Archive::hasComment() const
{
    return !m_iface->comment().isEmpty();
}

Archive::EncryptionType Archive::encryptionType()
{
    listIfNotListed();
    return m_encryptionType;
}

qulonglong Archive::numberOfFiles() const
{
    return m_numberOfFiles;
}

qulonglong Archive::unpackedSize() const
{
    return m_extractedFilesSize;
}

qulonglong Archive::packedSize() const
{
    return QFileInfo(fileName()).size();
}

QString Archive::subfolderName()
{
    listIfNotListed();
    return m_subfolderName;
}

void Archive::onNewEntry(const ArchiveEntry &entry)
{
    if (!entry[IsDirectory].toBool()) {
        m_numberOfFiles++;
    }
}

bool Archive::isValid() const
{
    return (m_error == NoError);
}

ArchiveError Archive::error() const
{
    return m_error;
}

KJob* Archive::open()
{
    return 0;
}

KJob* Archive::create()
{
    return 0;
}

ListJob* Archive::list()
{
    if (!QFileInfo::exists(fileName())) {
        return Q_NULLPTR;
    }

    qCDebug(ARK) << "Going to list files";

    ListJob *job = new ListJob(m_iface, this);
    job->setAutoDelete(false);

    //if this job has not been listed before, we grab the opportunity to
    //collect some information about the archive
    if (!m_hasBeenListed) {
        connect(job, &ListJob::result, this, &Archive::onListFinished);
    }
    return job;
}

DeleteJob* Archive::deleteFiles(const QList<QVariant> & files)
{
    qCDebug(ARK) << "Going to delete files" << files;

    if (m_iface->isReadOnly()) {
        return 0;
    }
    DeleteJob *newJob = new DeleteJob(files, static_cast<ReadWriteArchiveInterface*>(m_iface), this);

    return newJob;
}

AddJob* Archive::addFiles(const QStringList & files, const CompressionOptions& options)
{
    qCDebug(ARK) << "Going to add files" << files << "with options" << options;
    Q_ASSERT(!m_iface->isReadOnly());
    AddJob *newJob = new AddJob(files, options, static_cast<ReadWriteArchiveInterface*>(m_iface), this);
    connect(newJob, &AddJob::result, this, &Archive::onAddFinished);
    return newJob;
}

ExtractJob* Archive::copyFiles(const QList<QVariant>& files, const QString& destinationDir, const ExtractionOptions& options)
{
    ExtractionOptions newOptions = options;
    if (encryptionType() != Unencrypted) {
        newOptions[QStringLiteral( "PasswordProtectedHint" )] = true;
    }

    ExtractJob *newJob = new ExtractJob(files, destinationDir, newOptions, m_iface, this);
    return newJob;
}

void Archive::encrypt(const QString &password, bool encryptHeader)
{
    m_iface->setPassword(password);
    m_iface->setHeaderEncryptionEnabled(encryptHeader);
    m_encryptionType = encryptHeader ? HeaderEncrypted : Encrypted;
}

void Archive::onAddFinished(KJob* job)
{
    //if the archive was previously a single folder archive and an add job
    //has successfully finished, then it is no longer a single folder
    //archive (for the current implementation, which does not allow adding
    //folders/files other places than the root.
    //TODO: handle the case of creating a new file and singlefolderarchive
    //then.
    if (m_isSingleFolderArchive && !job->error()) {
        m_isSingleFolderArchive = false;
    }
}

void Archive::onListFinished(KJob* job)
{
    ListJob *ljob = qobject_cast<ListJob*>(job);
    m_extractedFilesSize = ljob->extractedFilesSize();
    m_isSingleFolderArchive = ljob->isSingleFolderArchive();
    m_subfolderName = ljob->subfolderName();
    if (m_subfolderName.isEmpty()) {
        m_subfolderName = completeBaseName();
    }

    if (ljob->isPasswordProtected()) {
        // If we already know the password, it means that the archive is header-encrypted.
        m_encryptionType = m_iface->password().isEmpty() ? Encrypted : HeaderEncrypted;
    }

    m_hasBeenListed = true;
}

void Archive::listIfNotListed()
{
    if (!m_hasBeenListed) {
        ListJob *job = list();
        if (!job) {
            return;
        }

        connect(job, &ListJob::userQuery, this, &Archive::onUserQuery);

        QEventLoop loop(this);

        connect(job, &KJob::result, &loop, &QEventLoop::quit);
        job->start();
        loop.exec(); // krazy:exclude=crashy
    }
}

void Archive::onUserQuery(Query* query)
{
    query->execute();
}

} // namespace Kerfuffle

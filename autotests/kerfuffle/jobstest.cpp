/*
 * Copyright (c) 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include "jsonarchiveinterface.h"
#include "kerfuffle/jobs.h"
#include "kerfuffle/archiveentry.h"

#include <QDebug>
#include <QEventLoop>
#include <QTest>

using namespace Kerfuffle;

class JobsTest : public QObject
{
    Q_OBJECT

public:
    JobsTest();

protected Q_SLOTS:
    void init();
    void slotNewEntry(Archive::Entry *entry);

private Q_SLOTS:
    // ListJob-related tests
    void testListJob_data();
    void testListJob();

    // ExtractJob-related tests
    void testExtractJobAccessors();
    void testTempExtractJob();

    // DeleteJob-related tests
    void testRemoveEntries_data();
    void testRemoveEntries();

    // AddJob-related tests
    void testAddEntries_data();
    void testAddEntries();

private:
    JSONArchiveInterface *createArchiveInterface(const QString& filePath);
    QList<Archive::Entry*> listEntries(JSONArchiveInterface *iface);
    void startAndWaitForResult(KJob *job);

    QList<Archive::Entry*> m_entries;
    QEventLoop m_eventLoop;
};

QTEST_GUILESS_MAIN(JobsTest)

JobsTest::JobsTest()
    : QObject(Q_NULLPTR)
    , m_eventLoop(this)
{
}

void JobsTest::init()
{
    m_entries.clear();
}

void JobsTest::slotNewEntry(Archive::Entry *entry)
{
    m_entries.append(entry);
}

JSONArchiveInterface *JobsTest::createArchiveInterface(const QString& filePath)
{
    JSONArchiveInterface *iface = new JSONArchiveInterface(this, {filePath});
    if (!iface->open()) {
        qDebug() << "Could not open" << filePath;
        return Q_NULLPTR;
    }

    return iface;
}

QList<Archive::Entry*> JobsTest::listEntries(JSONArchiveInterface *iface)
{
    m_entries.clear();

    ListJob *listJob = new ListJob(iface);
    connect(listJob, &Job::newEntry,
            this, &JobsTest::slotNewEntry);

    startAndWaitForResult(listJob);

    return m_entries;
}

void JobsTest::startAndWaitForResult(KJob *job)
{
    connect(job, &KJob::result, &m_eventLoop, &QEventLoop::quit);
    job->start();
    m_eventLoop.exec();
}

void JobsTest::testListJob_data()
{
    QTest::addColumn<QString>("jsonArchive");
    QTest::addColumn<qlonglong>("expectedExtractedFilesSize");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<QStringList>("expectedEntryNames");

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
            << 0LL << false << false
            << QStringList {QStringLiteral("a.txt"), QStringLiteral("aDir/"), QStringLiteral("aDir/b.txt"), QStringLiteral("c.txt")};

    QTest::newRow("archive002.json") << QFINDTESTDATA("data/archive002.json")
            << 45959LL << false << false
            << QStringList {QStringLiteral("a.txt"), QStringLiteral("aDir/"), QStringLiteral("aDir/b.txt"), QStringLiteral("c.txt")};

    QTest::newRow("archive-deepsinglehierarchy.json") << QFINDTESTDATA("data/archive-deepsinglehierarchy.json")
            << 0LL << false << true
            << QStringList {
                    // Depth-first order!
                    QStringLiteral("aDir/"),
                    QStringLiteral("aDir/aDirInside/"),
                    QStringLiteral("aDir/aDirInside/anotherDir/"),
                    QStringLiteral("aDir/aDirInside/anotherDir/file.txt"),
                    QStringLiteral("aDir/b.txt")
               };

    QTest::newRow("archive-multiplefolders.json") << QFINDTESTDATA("data/archive-multiplefolders.json")
            << 0LL << false << false
            << QStringList {QStringLiteral("aDir/"), QStringLiteral("aDir/b.txt"), QStringLiteral("anotherDir/"), QStringLiteral("anotherDir/file.txt")};

    QTest::newRow("archive-nodir-manyfiles.json") << QFINDTESTDATA("data/archive-nodir-manyfiles.json")
            << 0LL << false << false
            << QStringList {QStringLiteral("a.txt"), QStringLiteral("file.txt")};

    QTest::newRow("archive-onetopfolder.json") << QFINDTESTDATA("data/archive-onetopfolder.json")
            << 0LL << false << true
            << QStringList {QStringLiteral("aDir/"), QStringLiteral("aDir/b.txt")};

    QTest::newRow("archive-password.json") << QFINDTESTDATA("data/archive-password.json")
            << 0LL << true << false
            // Possibly unexpected behavior of listing:
            // 1. Directories are listed before files, if they are empty!
            // 2. Files are sorted alphabetically.
            << QStringList {QStringLiteral("aDirectory/"), QStringLiteral("bar.txt"), QStringLiteral("foo.txt")};

    QTest::newRow("archive-singlefile.json") << QFINDTESTDATA("data/archive-singlefile.json")
            << 0LL << false << false
            << QStringList {QStringLiteral("a.txt")};

    QTest::newRow("archive-emptysinglefolder.json") << QFINDTESTDATA("data/archive-emptysinglefolder.json")
            << 0LL << false << true
            << QStringList {QStringLiteral("aDir/")};

    QTest::newRow("archive-unorderedsinglefolder.json") << QFINDTESTDATA("data/archive-unorderedsinglefolder.json")
            << 0LL << false << true
            << QStringList {
                    QStringLiteral("aDir/"),
                    QStringLiteral("aDir/anotherDir/"),
                    QStringLiteral("aDir/anotherDir/bar.txt"),
                    QStringLiteral("aDir/foo.txt")
               };
}

void JobsTest::testListJob()
{
    QFETCH(QString, jsonArchive);
    JSONArchiveInterface *iface = createArchiveInterface(jsonArchive);
    QVERIFY(iface);

    ListJob *listJob = new ListJob(iface);
    listJob->setAutoDelete(false);
    startAndWaitForResult(listJob);

    QFETCH(qlonglong, expectedExtractedFilesSize);
    QCOMPARE(listJob->extractedFilesSize(), expectedExtractedFilesSize);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(listJob->isPasswordProtected(), isPasswordProtected);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(listJob->isSingleFolderArchive(), isSingleFolder);

    QFETCH(QStringList, expectedEntryNames);
    auto archiveEntries = listEntries(iface);

    QCOMPARE(archiveEntries.size(), expectedEntryNames.size());

    for (int i = 0; i < archiveEntries.size(); i++) {
        QCOMPARE(archiveEntries.at(i)->property("fullPath").toString(), expectedEntryNames.at(i));
    }

    listJob->deleteLater();
}

void JobsTest::testExtractJobAccessors()
{
    JSONArchiveInterface *iface = createArchiveInterface(QFINDTESTDATA("data/archive001.json"));
    ExtractJob *job = new ExtractJob(QList<Archive::Entry*>(), QStringLiteral("/tmp/some-dir"), ExtractionOptions(), iface);
    ExtractionOptions defaultOptions;
    defaultOptions[QStringLiteral("PreservePaths")] = false;

    QCOMPARE(job->destinationDirectory(), QLatin1String("/tmp/some-dir"));
    QCOMPARE(job->extractionOptions(), defaultOptions);

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/tmp/some-dir"));
    QCOMPARE(job->extractionOptions(), defaultOptions);
    delete job;

    ExtractionOptions options;
    options[QStringLiteral("PreservePaths")] = true;
    options[QStringLiteral("foo")] = QLatin1String("bar");
    options[QStringLiteral("pi")] = 3.14f;

    job = new ExtractJob(QList<Archive::Entry*>(), QStringLiteral("/root"), options, iface);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/root"));
    QCOMPARE(job->extractionOptions(), options);

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/root"));
    QCOMPARE(job->extractionOptions(), options);
    delete job;
}

void JobsTest::testTempExtractJob()
{
    JSONArchiveInterface *iface = createArchiveInterface(QFINDTESTDATA("data/archive-malicious.json"));
    PreviewJob *job = new PreviewJob(new Archive::Entry(this, QStringLiteral("anotherDir/../../file.txt")), false, iface);

    QVERIFY(job->validatedFilePath().endsWith(QLatin1String("anotherDir/file.txt")));
    QVERIFY(job->extractionOptions()[QStringLiteral("PreservePaths")].toBool());

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QVERIFY(job->validatedFilePath().endsWith(QLatin1String("anotherDir/file.txt")));
    QVERIFY(job->extractionOptions()[QStringLiteral("PreservePaths")].toBool());

    delete job;
}

void JobsTest::testRemoveEntries_data()
{
    QTest::addColumn<QString>("jsonArchive");
    QTest::addColumn<QList<Archive::Entry*>>("entries");
    QTest::addColumn<QList<Archive::Entry*>>("entriesToDelete");

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("a.txt")),
                new Archive::Entry(this, QStringLiteral("aDir/")),
                new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                new Archive::Entry(this, QStringLiteral("c.txt"))
            }
            << QList<Archive::Entry*> {new Archive::Entry(this, QStringLiteral("c.txt"))};

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("a.txt")),
                new Archive::Entry(this, QStringLiteral("aDir/")),
                new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                new Archive::Entry(this, QStringLiteral("c.txt"))
            }
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("a.txt")),
                new Archive::Entry(this, QStringLiteral("c.txt"))
            };

    // Error test: if we delete non-existent entries, the archive must not change.
    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("a.txt")),
                new Archive::Entry(this, QStringLiteral("aDir/")),
                new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                new Archive::Entry(this, QStringLiteral("c.txt"))
            }
            << QList<Archive::Entry*> {new Archive::Entry(this, QStringLiteral("foo.txt"))};
}

void JobsTest::testRemoveEntries()
{
    QFETCH(QString, jsonArchive);
    JSONArchiveInterface *iface = createArchiveInterface(jsonArchive);
    QVERIFY(iface);

    QFETCH(QList<Archive::Entry*>, entries);
    QFETCH(QList<Archive::Entry*>, entriesToDelete);
    QStringList fullPathsToDelete = iface->entryFullPaths(entriesToDelete);

    QList<Archive::Entry*> expectedRemainingEntries;
    Q_FOREACH (Archive::Entry *entry, entries) {
        if (!fullPathsToDelete.contains(entry->property("fullPath").toString())) {
            expectedRemainingEntries.append(entry);
        }
    }

    DeleteJob *deleteJob = new DeleteJob(entriesToDelete, iface);
    startAndWaitForResult(deleteJob);

    auto remainingEntries = listEntries(iface);
    QCOMPARE(remainingEntries.size(), expectedRemainingEntries.size());

    for (int i = 0; i < remainingEntries.size(); i++) {
        QCOMPARE(*remainingEntries.at(i), *expectedRemainingEntries.at(i));
    }

    iface->deleteLater();
}

void JobsTest::testAddEntries_data()
{
    QTest::addColumn<QString>("jsonArchive");
    QTest::addColumn<QList<Archive::Entry*>>("originalEntries");
    QTest::addColumn<QList<Archive::Entry*>>("entriesToAdd");

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("a.txt")),
                new Archive::Entry(this, QStringLiteral("aDir/")),
                new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                new Archive::Entry(this, QStringLiteral("c.txt"))
            }
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("foo.txt"))
            };

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("a.txt")),
                new Archive::Entry(this, QStringLiteral("aDir/")),
                new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                new Archive::Entry(this, QStringLiteral("c.txt"))
            }
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("foo.txt")),
                new Archive::Entry(this, QStringLiteral("bar.txt"))
            };

    // Error test: if we add an already existent entry, the archive must not change.
    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
            << QList<Archive::Entry*> {
                new Archive::Entry(this, QStringLiteral("a.txt")),
                new Archive::Entry(this, QStringLiteral("aDir/")),
                new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                new Archive::Entry(this, QStringLiteral("c.txt"))
            }
            << QList<Archive::Entry*> {new Archive::Entry(this, QStringLiteral("c.txt"))};
}

void JobsTest::testAddEntries()
{
    QFETCH(QString, jsonArchive);
    JSONArchiveInterface *iface = createArchiveInterface(jsonArchive);
    QVERIFY(iface);

    QFETCH(QList<Archive::Entry*>, originalEntries);
    QStringList originalFullPaths = QStringList();
    Q_FOREACH (Archive::Entry *entry, originalEntries) {
        originalFullPaths.append(entry->property("fullPath").toString());
    }
    auto currentEntries = listEntries(iface);
    QCOMPARE(currentEntries.size(), originalEntries.size());

    QFETCH(QList<Archive::Entry*>, entriesToAdd);
    AddJob *addJob = new AddJob(entriesToAdd, CompressionOptions(), iface);
    startAndWaitForResult(addJob);

    currentEntries = listEntries(iface);

    int expectedEntriesCount = originalEntries.size();
    Q_FOREACH (Archive::Entry *entry, entriesToAdd) {
        if (!originalFullPaths.contains(entry->property("fullPath").toString())) {
            expectedEntriesCount++;
        }
    }

    QCOMPARE(currentEntries.size(), expectedEntriesCount);

    iface->deleteLater();
}

#include "jobstest.moc"

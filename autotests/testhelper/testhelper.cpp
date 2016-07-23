//
// Created by mvlabat on 7/21/16.
//

#include "testhelper.h"

QEventLoop TestHelper::m_eventLoop;

void TestHelper::startAndWaitForResult(KJob *job)
{
    QObject::connect(job, &KJob::result, &m_eventLoop, &QEventLoop::quit);
    //qDebug() << 1;
    job->start();
    //qDebug() << 1;
    m_eventLoop.exec();
    //qDebug() << 1;
}

QList<Archive::Entry*> TestHelper::getEntryList(ReadOnlyArchiveInterface *iface)
{
    QList<Archive::Entry*> list = QList<Archive::Entry*>();
    ListJob *listJob = new ListJob(iface);
    QObject::connect(listJob, &Job::newEntry, [&list](Archive::Entry* entry) { list << entry; });
    //qDebug() << 1;
    startAndWaitForResult(listJob);
    qDebug() << 1;
    listJob->deleteLater();
    //qDebug() << 1;
    return list;
}

QStringList TestHelper::getExpectedEntryPaths(const QList<Archive::Entry*> &entryList, const Archive::Entry *destination)
{
    QStringList expectedPaths = QStringList();
    foreach (const Archive::Entry *entry, entryList) {
        expectedPaths << destination->property("fullPath").toString() + entry->property("fullPath").toString();
    }
    return expectedPaths;
}

void TestHelper::verifyEntriesWithDestination(const QList<Archive::Entry*> &oldEntries, const Archive::Entry *destination, const QList<Archive::Entry*> &newEntries)
{
    QStringList expectedPaths = getExpectedEntryPaths(oldEntries, destination);
    QStringList actualPaths = ReadOnlyArchiveInterface::entryFullPaths(newEntries);
    foreach (const QString &path, expectedPaths) {
        QVERIFY2(actualPaths.contains(path), (QStringLiteral("No ") + path + QStringLiteral(" inside the archive")).toStdString().c_str());
    }
}

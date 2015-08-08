/*
 * Copyright (c) 2011,2014 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "clirartest.h"

#include <QtTest/QtTest>

#include <QFile>
#include <QTextStream>

QTEST_GUILESS_MAIN(CliRarTest)

using namespace Kerfuffle;

/*
 * Check that the plugin will not crash when reading corrupted archives, which
 * have lines such as "Unexpected end of archive" or "??? - the file header is
 * corrupt" instead of a file name and the header string after it.
 *
 * See bug 262857 and commit 2042997013432cdc6974f5b26d39893a21e21011.
 */
void CliRarTest::testReadCorruptedArchive()
{
    qRegisterMetaType<ArchiveEntry>();

    QVariantList args;
    args.append(QStringLiteral("DummyArchive.rar"));

    CliPlugin *rarPlugin = new CliPlugin(this, args);
    QVERIFY(rarPlugin->open());

    QSignalSpy spy(rarPlugin, SIGNAL(entry(ArchiveEntry)));

    QFile unrarOutput(QFINDTESTDATA("data/testReadCorruptedArchive.txt"));
    QVERIFY(unrarOutput.open(QIODevice::ReadOnly));

    QTextStream unrarStream(&unrarOutput);
    while (!unrarStream.atEnd()) {
        const QString line(unrarStream.readLine());
        QVERIFY(rarPlugin->readListLine(line));
    }

    qDebug() << "Entries found:" << spy.count();
    QCOMPARE(spy.count(), 1);

    // Check if the first entry's name is correctly parsed.
    ArchiveEntry firstEntry = qvariant_cast<ArchiveEntry>(spy.at(0).at(0));
    QCOMPARE(firstEntry[FileName].toString(), QString("some-file.ext"));

    rarPlugin->deleteLater();
}

/*
 * Bug 314297: do not crash when a RAR archive has a symlink.
 */
void CliRarTest::testParseSymlink()
{
    QVariantList args;
    args.append(QStringLiteral("DummyArchive.rar"));

    CliPlugin *rarPlugin = new CliPlugin(this, args);
    QVERIFY(rarPlugin->open());

    QSignalSpy spy(rarPlugin, SIGNAL(entry(ArchiveEntry)));

    QFile unrarOutput(QFINDTESTDATA("data/testReadArchiveWithSymlink.txt"));
    QVERIFY(unrarOutput.open(QIODevice::ReadOnly));

    QTextStream unrarStream(&unrarOutput);
    while (!unrarStream.atEnd()) {
        const QString line(unrarStream.readLine());
        QVERIFY(rarPlugin->readListLine(line));
    }

    qDebug() << "Entries found:" << spy.count();
    QCOMPARE(spy.count(), 3);

    // Check if the second entry's name is correctly parsed.
    ArchiveEntry secondEntry = qvariant_cast<ArchiveEntry>(spy.at(1).at(0));
    QCOMPARE(secondEntry[FileName].toString(), QString("foo/hello2"));

    rarPlugin->deleteLater();
}

/*
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

#include "libarchivetest.h"

#include <QSignalSpy>

#include <KPluginLoader>

QTEST_GUILESS_MAIN(LibarchiveTest)

using namespace Kerfuffle;

void LibarchiveTest::initTestCase()
{
    m_plugin = new Plugin(this);
    foreach (Plugin *plugin, m_pluginManger.availablePlugins()) {
        if (plugin->metaData().pluginId() == QStringLiteral("kerfuffle_cli7z")) {
            m_plugin = plugin;
            return;
        }
    }
}

void LibarchiveTest::testAdd_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QList<Archive::Entry*>>("files");
    QTest::addColumn<Archive::Entry*>("destination");

    QTest::newRow("without destination")
        << QStringLiteral("test.7z")
        << QList<Archive::Entry*> {
        new Archive::Entry(this, QStringLiteral("a.txt")),
        new Archive::Entry(this, QStringLiteral("b.txt")),
    }
        << new Archive::Entry(this);

    QTest::newRow("with destination, files")
        << QStringLiteral("test.7z")
        << QList<Archive::Entry*> {
        new Archive::Entry(this, QStringLiteral("a.txt")),
        new Archive::Entry(this, QStringLiteral("b.txt")),
    }
        << new Archive::Entry(this, QStringLiteral("dir/"));

    QTest::newRow("with destination, directory")
        << QStringLiteral("test.7z")
        << QList<Archive::Entry*> {
        new Archive::Entry(this, QStringLiteral("dir/")),
    }
        << new Archive::Entry(this, QStringLiteral("dir/"));
}

void LibarchiveTest::testAdd()
{
    QTemporaryDir temporaryDir;

    QFETCH(QString, archiveName);
    ReadWriteLibarchivePlugin *plugin = new ReadWriteLibarchivePlugin(this, {QVariant(temporaryDir.path() + QLatin1Char('/') + archiveName)});
    QVERIFY(plugin);

    QFETCH(QList<Archive::Entry*>, files);
    QFETCH(Archive::Entry*, destination);

    CompressionOptions options = CompressionOptions();
    options.insert(QStringLiteral("GlobalWorkDir"), QFINDTESTDATA("data"));
    AddJob *addJob = new AddJob(files, destination, options, plugin);
    TestHelper::startAndWaitForResult(addJob);
    addJob->deleteLater();

    QList<Archive::Entry*> resultedEntries = TestHelper::getEntryList(plugin);
    TestHelper::verifyEntriesWithDestination(files, destination, resultedEntries);

    plugin->deleteLater();
}

/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include "ark_debug.h"
#include "plugin.h"

#include <QJsonArray>
#include <QStandardPaths>

namespace Kerfuffle
{

Plugin::Plugin(QObject *parent, const KPluginMetaData &metaData)
    : QObject(parent)
    , m_enabled(true)
    , m_metaData(metaData)
{
}

unsigned int Plugin::priority() const
{
    const int priority = m_metaData.rawData()[QStringLiteral("X-KDE-Priority")].toInt();
    return (priority > 0 ? priority : 0);
}

bool Plugin::isEnabled() const
{
    return m_enabled;
}

void Plugin::setEnabled(bool enabled)
{
    m_enabled = enabled;
    emit enabledChanged();
}

bool Plugin::isReadWrite() const
{
    const bool isDeclaredReadWrite = m_metaData.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWrite")].toBool();
    return isDeclaredReadWrite && findExecutables(readWriteExecutables());
}

QStringList Plugin::readOnlyExecutables() const
{
    QStringList readOnlyExecutables;

    const QJsonArray array = m_metaData.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadOnlyExecutables")].toArray();
    foreach (const QJsonValue &value, array) {
        readOnlyExecutables << value.toString();
    }

    return readOnlyExecutables;
}

QStringList Plugin::readWriteExecutables() const
{
    QStringList readWriteExecutables;

    const QJsonArray array = m_metaData.rawData()[QStringLiteral("X-KDE-Kerfuffle-ReadWriteExecutables")].toArray();
    foreach (const QJsonValue &value, array) {
        readWriteExecutables << value.toString();
    }

    return readWriteExecutables;
}

KPluginMetaData Plugin::metaData() const
{
    return m_metaData;
}

bool Plugin::isValid() const
{
    return isEnabled() && m_metaData.isValid() && findExecutables(readOnlyExecutables());
}

bool Plugin::findExecutables(const QStringList &executables)
{
    foreach (const QString &executable, executables) {
        if (executable.isEmpty()) {
            continue;
        }

        if (QStandardPaths::findExecutable(executable).isEmpty()) {
            qCDebug(ARK) << "Could not find executable" << executable;
            return false;
        }
    }

    return true;
}

}

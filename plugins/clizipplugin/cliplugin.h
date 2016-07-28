/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "kerfuffle/cliinterface.h"

#include <QTemporaryDir>

using namespace Kerfuffle;

class KERFUFFLE_EXPORT CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList &args);
    virtual ~CliPlugin();

    virtual void resetParsing() Q_DECL_OVERRIDE;
    virtual QString escapeFileName(const QString &fileName) const Q_DECL_OVERRIDE;
    virtual Kerfuffle::ParameterList parameterList() const Q_DECL_OVERRIDE;
    virtual bool readListLine(const QString &line) Q_DECL_OVERRIDE;

    virtual bool moveFiles(const QList<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions& options) Q_DECL_OVERRIDE;
    virtual int moveRequiredSignals() const Q_DECL_OVERRIDE;

private:
    void continueMoving(bool result);
    void setAddedFiles();
    void cleanUpMoving();

    enum ParseState {
        ParseStateHeader = 0,
        ParseStateComment,
        ParseStateEntry
    } m_parseState;

    QTemporaryDir *m_moveExtractedDir;
    QTemporaryDir *m_moveAddedDir;
    OperationMode m_moveSubOperation;
    QList<Archive::Entry*> m_movedFiles;
    QList<Archive::Entry*> m_moveAddedFiles;
    Archive::Entry *m_moveDestination;
    CompressionOptions m_moveCompressionOptions;

    int m_linesComment;
    QString m_tempComment;
};

#endif // CLIPLUGIN_H

/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "compressionoptionswidget.h"
#include "archiveformat.h"
#include "mimetypes.h"

#include <KPluginMetaData>

#include <QMimeDatabase>

namespace Kerfuffle
{
class CompressionOptionsUI: public QWidget, public Ui::CompressionOptions
{
public:
    CompressionOptionsUI(QWidget *parent = 0)
            : QWidget(parent) {
        setupUi(this);
    }
};

CompressionOptionsWidget::CompressionOptionsWidget(const QMimeType &mimeType, QWidget *parent)
    : QWidget(parent)
{
    m_ui = new CompressionOptionsUI(this);

    const KPluginMetaData metadata = preferredPluginFor(mimeType, Kerfuffle::supportedWritePlugins());
    const ArchiveFormat archiveFormat = ArchiveFormat::fromMetadata(mimeType, metadata);
    Q_ASSERT(archiveFormat.isValid());

    if (archiveFormat.encryptionType() != Archive::Unencrypted) {
        m_ui->collapsibleEncryption->setEnabled(true);
        m_ui->collapsibleEncryption->setToolTip(QString());
    } else {
        m_ui->collapsibleEncryption->setEnabled(false);
        m_ui->collapsibleEncryption->setToolTip(i18n("Protection of the archive with password is not possible with the %1 format.",
                                                mimeType.comment()));
    }

    if (archiveFormat.maxCompressionLevel() == 0) {
        m_ui->collapsibleCompression->setEnabled(false);
    } else {
        m_ui->collapsibleCompression->setEnabled(true);
        m_ui->compLevelSlider->setMinimum(archiveFormat.minCompressionLevel());
        m_ui->compLevelSlider->setMaximum(archiveFormat.maxCompressionLevel());
        m_ui->compLevelSlider->setValue(archiveFormat.defaultCompressionLevel());
    }
}
}


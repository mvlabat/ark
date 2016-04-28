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
#include "ark_debug.h"
#include "archiveformat.h"
#include "pluginmanager.h"

#include <KColorScheme>
#include <KPluginMetaData>

#include <QMimeDatabase>

namespace Kerfuffle
{
CompressionOptionsWidget::CompressionOptionsWidget(const QMimeType &mimeType,
                                                   const CompressionOptions &opts,
                                                   QWidget *parent)
    : QWidget(parent)
    , m_mimetype(mimeType)
{
    setupUi(this);

    qCDebug(ARK) << "opts:" << opts;

    KColorScheme colorScheme(QPalette::Active, KColorScheme::View);
    pwdWidget->setBackgroundWarningColor(colorScheme.background(KColorScheme::NegativeBackground).color());
    pwdWidget->setPasswordStrengthMeterVisible(false);

    const KPluginMetaData metadata = PluginManager().preferredPluginFor(mimeType)->metaData();
    const ArchiveFormat archiveFormat = ArchiveFormat::fromMetadata(mimeType, metadata);
    Q_ASSERT(archiveFormat.isValid());

    if (archiveFormat.encryptionType() != Archive::Unencrypted) {
        collapsibleEncryption->setEnabled(true);
        collapsibleEncryption->setToolTip(QString());
    } else {
        collapsibleEncryption->setEnabled(false);
        collapsibleEncryption->setToolTip(i18n("Protection of the archive with password is not possible with the %1 format.",
                                               mimeType.comment()));
    }

    if (archiveFormat.maxCompressionLevel() == 0) {
        collapsibleCompression->setEnabled(false);
    } else {
        collapsibleCompression->setEnabled(true);
        compLevelSlider->setMinimum(archiveFormat.minCompressionLevel());
        compLevelSlider->setMaximum(archiveFormat.maxCompressionLevel());
    }

    if (opts.contains(QStringLiteral("CompressionLevel"))) {
        compLevelSlider->setValue(opts.value(QStringLiteral("CompressionLevel")).toInt());
    } else {
        compLevelSlider->setValue(archiveFormat.defaultCompressionLevel());
    }

    connect(collapsibleEncryption, &KCollapsibleGroupBox::expandedChanged, this, &CompressionOptionsWidget::slotEncryptionToggled);

    slotEncryptionToggled();
}

CompressionOptions CompressionOptionsWidget::commpressionOptions() const
{
    CompressionOptions opts;
    opts[QStringLiteral("CompressionLevel")] = compLevelSlider->value();

    return opts;
}

int CompressionOptionsWidget::compressionLevel() const
{
    return compLevelSlider->value();
}

void CompressionOptionsWidget::setEncryptionVisible(bool visible)
{
    collapsibleEncryption->setVisible(visible);
}

QString CompressionOptionsWidget::password() const
{
    return pwdWidget->password();
}

void CompressionOptionsWidget::slotEncryptionToggled()
{
    const KPluginMetaData metadata = PluginManager().preferredPluginFor(m_mimetype)->metaData();
    const ArchiveFormat archiveFormat = ArchiveFormat::fromMetadata(m_mimetype, metadata);
    Q_ASSERT(archiveFormat.isValid());

    const bool isExpanded = collapsibleEncryption->isExpanded();
    if (isExpanded && (archiveFormat.encryptionType() == Archive::HeaderEncrypted)) {
        encryptHeaderCheckBox->setEnabled(true);
        encryptHeaderCheckBox->setToolTip(QString());
    } else {
        encryptHeaderCheckBox->setEnabled(false);
        // Show the tooltip only if the encryption is still enabled.
        // This is needed because if the new filter is e.g. tar, the whole encryption group gets disabled.
        if (collapsibleEncryption->isEnabled() && collapsibleEncryption->isExpanded()) {
            encryptHeaderCheckBox->setToolTip(i18n("Protection of the list of files is not possible with the %1 format.",
                                                   m_mimetype.comment()));
        } else {
            encryptHeaderCheckBox->setToolTip(QString());
        }
    }
    pwdWidget->setEnabled(isExpanded);
}

}

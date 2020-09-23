/*
 * Copyright (C) 2016 Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "kra_import.h"

#include <kpluginfactory.h>
#include <QFileInfo>

#include <KisDocument.h>
#include <kis_image.h>

#include "kra_converter.h"

K_PLUGIN_FACTORY_WITH_JSON(ImportFactory, "krita_kra_import.json", registerPlugin<KraImport>();)

KraImport::KraImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KraImport::~KraImport()
{
}

KisImportExportErrorCode KraImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP /*configuration*/)
{
    KraConverter kraConverter(document);
    KisImportExportErrorCode result = kraConverter.buildImage(io);
    if (result.isOk()) {
        document->setCurrentImage(kraConverter.image());
        if (kraConverter.activeNodes().size() > 0) {
            document->setPreActivatedNode(kraConverter.activeNodes()[0]);
        }
        if (kraConverter.assistants().size() > 0) {
            document->setAssistants(kraConverter.assistants());
        }
        if (kraConverter.storyboardItemList().size() > 0) {
            document->setStoryboardItemList(kraConverter.storyboardItemList(), true);
        }
        if (kraConverter.storyboardCommentList().size() > 0) {
            document->setStoryboardCommentList(kraConverter.storyboardCommentList(), true);
        }
    }
    return result;
}

#include <kra_import.moc>


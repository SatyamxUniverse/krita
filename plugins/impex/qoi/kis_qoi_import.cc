/*
 *  SPDX-FileCopyrightText: 2024 Grum999 <grum999@grum.fr>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_qoi_import.h"

#include <kpluginfactory.h>
#include <QFileInfo>

#include <KisImportExportManager.h>

#include <KisDocument.h>
#include <kis_transaction.h>
#include <kis_paint_device.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_node.h>
#include <kis_group_layer.h>

#include <KisViewManager.h>

#include "kis_qoi_converter.h"

K_PLUGIN_FACTORY_WITH_JSON(PNGImportFactory, "krita_qoi_import.json", registerPlugin<KisQOIImport>();)

KisQOIImport::KisQOIImport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisQOIImport::~KisQOIImport()
{
}

KisImportExportErrorCode KisQOIImport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP configuration)
{
    Q_UNUSED(configuration);

    KisQOIConverter converter(document, batchMode());

    KisImportExportErrorCode result = converter.read(io);
    if(result == ImportExportCodes::OK) {
        const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
        KisImageSP image = new KisImage(document->createUndoStore(), converter.image().width(), converter.image().height(), colorSpace, "imported from qoi");
        KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), 255);
        layer->paintDevice()->convertFromQImage(converter.image(), 0, 0, 0);
        image->addNode(layer.data(), image->rootLayer().data());
        document->setCurrentImage(image);
    }
    return result;
}

#include <kis_qoi_import.moc>


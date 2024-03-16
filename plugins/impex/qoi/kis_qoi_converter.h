/*
 *  SPDX-FileCopyrightText: 2024 Grum999 <grum999@grum.fr>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _KIS_QOI_CONVERTER_H_
#define _KIS_QOI_CONVERTER_H_

#include <QColor>

#include "KoColorSpace.h"
#include "kis_types.h"
#include "kis_global.h"
#include <kritaui_export.h>
#include <KisImportExportErrorCode.h>

class KisDocument;

namespace KisMetaData
{
class Filter;
class Store;
}

struct KisQOIOptions {
    KisQOIOptions()
        : alpha(true)
        , transparencyFillColor(Qt::white)
    {}

    bool alpha;
    QColor transparencyFillColor;
};

/**
 * This class allows to import/export a QOI from either a file or a QIODevice.
 */
class KRITAUI_EXPORT KisQOIConverter : public QObject
{
    Q_OBJECT
public:
    /**
     * Initialize the converter.
     * @param doc the KisDocument related to the image, can be null if you don't have a KisDocument
     * @param batchMode whether to use the batch mode
     */
    KisQOIConverter(KisDocument *doc, bool batchMode = false);
    ~KisQOIConverter() override;
public:
    /**
     * Retrieve the constructed image
     */
    QImage image();

    KisImportExportErrorCode read(QIODevice *io);
    KisImportExportErrorCode write(QIODevice *io, const QImage &image);

    bool isColorSpaceSupported(const KoColorSpace *cs);
private:
    struct Private;
    QScopedPointer<Private> m_d;
};

#endif

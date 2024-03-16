/*
 *  SPDX-FileCopyrightText: 2024 Grum999 <grum999@grum.fr>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef _KIS_QOI_IMPORT_H_
#define _KIS_QOI_IMPORT_H_

#include <QVariant>

#include <KisImportExportFilter.h>

class KisQOIImport : public KisImportExportFilter
{
    Q_OBJECT
public:
    KisQOIImport(QObject *parent, const QVariantList &);
    ~KisQOIImport() override;
public:
    KisImportExportErrorCode convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP configuration = 0) override;
};

#endif

/*
 *  SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef _PSD_EXPORT_H_
#define _PSD_EXPORT_H_

#include <QVariant>

#include "ui_options_psd.h"

#include <KisImportExportFilter.h>
#include <kis_config_widget.h>

class KisOptionsPSD : public KisConfigWidget, public Ui::OptionsPSD
{
    Q_OBJECT

public:
    KisOptionsPSD(QWidget *parent);

    void setConfiguration(const KisPropertiesConfigurationSP  config) override;
    KisPropertiesConfigurationSP configuration() const override;
};

class psdExport : public KisImportExportFilter {
    Q_OBJECT
    public:
        psdExport(QObject *parent, const QVariantList &);
        ~psdExport() override;
    public:
        KisImportExportErrorCode convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP configuration = 0) override;
        
        KisPropertiesConfigurationSP defaultConfiguration(const QByteArray& from = "", const QByteArray& to = "") const override;
        KisConfigWidget *createConfigurationWidget(QWidget *parent, const QByteArray& from = "", const QByteArray& to = "") const override;
        void initializeCapabilities() override;
};

#endif

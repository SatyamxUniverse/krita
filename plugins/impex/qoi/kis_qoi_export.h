/*
 *  SPDX-FileCopyrightText: 2024 Grum999 <grum999@grum.fr>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _KIS_QOI_EXPORT_H_
#define _KIS_QOI_EXPORT_H_

#include <QVariant>

#include <KisImportExportFilter.h>
#include <kis_config_widget.h>
#include "ui_kis_wdg_options_qoi.h"
#include <kis_meta_data_store.h>
#include <kis_meta_data_filter_registry_model.h>


class KisWdgOptionsQOI : public KisConfigWidget, public Ui::KisWdgOptionsQOI
{
    Q_OBJECT

public:
    KisWdgOptionsQOI(QWidget *parent);
    void setConfiguration(const KisPropertiesConfigurationSP  config) override;
    KisPropertiesConfigurationSP configuration() const override;

private Q_SLOTS:
    void on_alpha_toggled(bool checked);

private:
    KisMetaData::FilterRegistryModel m_filterRegistryModel;

};

class KisQOIExport : public KisImportExportFilter
{
    Q_OBJECT
public:
    KisQOIExport(QObject *parent, const QVariantList &);
    ~KisQOIExport() override;
public:
    KisImportExportErrorCode convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP configuration = 0) override;

    KisPropertiesConfigurationSP defaultConfiguration(const QByteArray& from = "", const QByteArray& to = "") const override;
    KisConfigWidget *createConfigurationWidget(QWidget *parent, const QByteArray& from = "", const QByteArray& to = "") const override;
    void initializeCapabilities() override;
};

#endif

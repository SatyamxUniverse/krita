/*
 *  SPDX-FileCopyrightText: 2024 Grum999 <grum999@grum.fr>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_qoi_export.h"

#include <QCheckBox>
#include <QApplication>

#include <kpluginfactory.h>

#include <KoColorSpace.h>
#include <KoColorProfile.h>
#include <KoColorModelStandardIds.h>
#include <KoColorSpaceRegistry.h>

#include <KisImportExportManager.h>
#include <KisImportExportErrorCode.h>


#include <kis_properties_configuration.h>
#include <kis_paint_device.h>
#include <KisDocument.h>
#include <kis_image.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>
#include <kis_config.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_filter_registry_model.h>
#include "kis_qoi_converter.h"
#include <kis_painter.h>

#include <KisExportCheckRegistry.h>

K_PLUGIN_FACTORY_WITH_JSON(KisQOIExportFactory, "krita_qoi_export.json", registerPlugin<KisQOIExport>();)

KisQOIExport::KisQOIExport(QObject *parent, const QVariantList &) : KisImportExportFilter(parent)
{
}

KisQOIExport::~KisQOIExport()
{
}

KisImportExportErrorCode KisQOIExport::convert(KisDocument *document, QIODevice *io,  KisPropertiesConfigurationSP configuration)
{
    KisQOIOptions options;
    KoColor c(KoColorSpaceRegistry::instance()->rgb8());
    c.fromQColor(Qt::white);
    options.alpha = configuration->getBool("alpha", true);
    options.transparencyFillColor = configuration->getColor("transparencyFillcolor", c).toQColor();

    QRect imageRect = document->savingImage()->bounds();
    KisPaintDeviceSP device = document->savingImage()->projection();

    if (!options.alpha) {
        KisPaintDeviceSP tmp = new KisPaintDevice(device->colorSpace());
        KoColor c(options.transparencyFillColor, device->colorSpace());
        tmp->fill(imageRect, c);
        KisPainter gc(tmp);
        gc.bitBlt(imageRect.topLeft(), device, imageRect);
        gc.end();
        device = tmp;
    }

    QImage image = device->convertToQImage(0, 0, 0, imageRect.width(), imageRect.height(), KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());

    // not sure why we need to swap red & blue channels?
    const QImage& img = image.rgbSwapped();
    KisQOIConverter converter = KisQOIConverter(document, batchMode());

    KisImportExportErrorCode result = converter.write(io, img);

    dbgFile << " Result =" << result;
    return result;
}

KisPropertiesConfigurationSP KisQOIExport::defaultConfiguration(const QByteArray &, const QByteArray &) const
{
    KisPropertiesConfigurationSP cfg = new KisPropertiesConfiguration();

    KoColor fill_color(KoColorSpaceRegistry::instance()->rgb8());
    fill_color = KoColor();
    fill_color.fromQColor(Qt::white);
    QVariant v;
    v.setValue(fill_color);

    cfg->setProperty("alpha", true);
    cfg->setProperty("transparencyFillcolor", v);
    return cfg;
}

KisConfigWidget *KisQOIExport::createConfigurationWidget(QWidget *parent, const QByteArray &, const QByteArray &) const
{
    return new KisWdgOptionsQOI(parent);
}

void KisQOIExport::initializeCapabilities()
{
    addCapability(KisExportCheckRegistry::instance()->get("sRGBProfileCheck")->create(KisExportCheckBase::SUPPORTED));
    QList<QPair<KoID, KoID> > supportedColorModels;
    supportedColorModels << QPair<KoID, KoID>()
            << QPair<KoID, KoID>(RGBAColorModelID, Integer8BitsColorDepthID);
    addSupportedColorModels(supportedColorModels, "QOI");
}

KisWdgOptionsQOI::KisWdgOptionsQOI(QWidget *parent)
    : KisConfigWidget(parent)
{
    setupUi(this);
}

void KisWdgOptionsQOI::setConfiguration(const KisPropertiesConfigurationSP cfg)
{
    // the export manager should have prepared some info for us!
    KIS_SAFE_ASSERT_RECOVER_NOOP(cfg->hasProperty(KisImportExportFilter::ImageContainsTransparencyTag));
    KIS_SAFE_ASSERT_RECOVER_NOOP(cfg->hasProperty(KisImportExportFilter::ColorModelIDTag));

    const bool isThereAlpha = cfg->getBool(KisImportExportFilter::ImageContainsTransparencyTag);

    alpha->setChecked(cfg->getBool("alpha", isThereAlpha));

    bnTransparencyFillColor->setEnabled(!alpha->isChecked());

    KoColor background(KoColorSpaceRegistry::instance()->rgb8());
    background.fromQColor(Qt::white);
    bnTransparencyFillColor->setDefaultColor(background);
    bnTransparencyFillColor->setColor(cfg->getColor("transparencyFillcolor", background));
}

KisPropertiesConfigurationSP KisWdgOptionsQOI::configuration() const
{
    KisPropertiesConfigurationSP cfg(new KisPropertiesConfiguration());

    bool alpha = this->alpha->isChecked();

    QVariant transparencyFillcolor;
    transparencyFillcolor.setValue(bnTransparencyFillColor->color());

    cfg->setProperty("alpha", alpha);
    cfg->setProperty("transparencyFillcolor", transparencyFillcolor);
    return cfg;
}

void KisWdgOptionsQOI::on_alpha_toggled(bool checked)
{
    bnTransparencyFillColor->setEnabled(!checked);
}


#include "kis_qoi_export.moc"


/*
 * This file is part of Krita
 *
 * SPDX-FileCopyrightText: 2018 Jouni Pentikainen <joupent@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_multichannel_filter_base.h"

#include <Qt>
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QLabel>
#include <QComboBox>
#include <QDomDocument>
#include <QHBoxLayout>
#include <QMessageBox>

#include "KoChannelInfo.h"
#include "KoBasicHistogramProducers.h"
#include "KoColorModelStandardIds.h"
#include "KoColorSpace.h"
#include "KoMixColorsOp.h"
#include "KoColorConversions.h"
#include "KoColorTransformation.h"
#include "KoCompositeColorTransformation.h"
#include "KoCompositeOp.h"
#include "KoID.h"

#include "kis_signals_blocker.h"
#include "kis_icon_utils.h"
#include "kis_bookmarked_configuration_manager.h"
#include "kis_config_widget.h"
#include <filter/kis_filter_category_ids.h>
#include <filter/kis_filter_configuration.h>
#include <kis_selection.h>
#include <kis_paint_device.h>
#include <kis_processing_information.h>

#include "kis_histogram.h"
#include "kis_painter.h"
#include "widgets/kis_curve_widget.h"

#include "kis_multichannel_utils.h"

KisMultiChannelFilter::KisMultiChannelFilter(const KoID& id, const QString &entry)
        : KisColorTransformationFilter(id, FiltersCategoryAdjustId, entry)
{
    setSupportsPainting(true);
    setColorSpaceIndependence(TO_LAB16);
}

bool KisMultiChannelFilter::needsTransparentPixels(const KisFilterConfigurationSP config, const KoColorSpace *cs) const
{
    Q_UNUSED(config);
    return cs->colorModelId() == AlphaColorModelID;
}

QVector<VirtualChannelInfo> KisMultiChannelFilter::getVirtualChannels(const KoColorSpace *cs, int maxChannels)
{
    return KisMultiChannelUtils::getVirtualChannels(cs, maxChannels);
}

int KisMultiChannelFilter::findChannel(const QVector<VirtualChannelInfo> &virtualChannels,
                                       const VirtualChannelInfo::Type &channelType)
{
    return KisMultiChannelUtils::findChannel(virtualChannels, channelType);
}


KisMultiChannelFilterConfiguration::KisMultiChannelFilterConfiguration(int channelCount, const QString & name, qint32 version, KisResourcesInterfaceSP resourcesInterface)
        : KisColorTransformationConfiguration(name, version, resourcesInterface)
        , m_channelCount(channelCount)
{
    m_transfers.resize(m_channelCount);
}

KisMultiChannelFilterConfiguration::KisMultiChannelFilterConfiguration(const KisMultiChannelFilterConfiguration &rhs)
    : KisColorTransformationConfiguration(rhs),
      m_channelCount(rhs.m_channelCount),
      m_curves(rhs.m_curves),
      m_transfers(rhs.m_transfers)
{
}

KisMultiChannelFilterConfiguration::~KisMultiChannelFilterConfiguration()
{}

void KisMultiChannelFilterConfiguration::init()
{
    m_curves.clear();
    for (int i = 0; i < m_channelCount; ++i) {
        m_curves.append(getDefaultCurve());
    }
    updateTransfers();
}

bool KisMultiChannelFilterConfiguration::isCompatible(const KisPaintDeviceSP dev) const
{
    return (int)dev->compositionSourceColorSpace()->channelCount() == m_channelCount;
}

void KisMultiChannelFilterConfiguration::setCurves(QList<KisCubicCurve> &curves)
{
    m_curves.clear();
    m_curves = curves;
    m_channelCount = curves.size();

    updateTransfers();
}

void KisMultiChannelFilterConfiguration::updateTransfer(int index)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(index >= 0 && index < m_curves.size());
    m_transfers[index] = m_curves[index].uint16Transfer();
}

void KisMultiChannelFilterConfiguration::updateTransfers()
{
    m_transfers.resize(m_channelCount);
    for (int i = 0; i < m_channelCount; i++) {
        m_transfers[i] = m_curves[i].uint16Transfer();
    }
}

const QVector<QVector<quint16> >&
KisMultiChannelFilterConfiguration::transfers() const
{
    return m_transfers;
}

const QList<KisCubicCurve>&
KisMultiChannelFilterConfiguration::curves() const
{
    return m_curves;
}

void KisMultiChannelFilterConfiguration::fromLegacyXML(const QDomElement& root)
{
    fromXML(root);
}

void KisMultiChannelFilterConfiguration::fromXML(const QDomElement& root)
{
    QList<KisCubicCurve> curves;
    quint16 numTransfers = 0;
    int version;
    version = root.attribute("version").toInt();

    QDomElement e = root.firstChild().toElement();
    QString attributeName;
    KisCubicCurve curve;
    quint16 index;
    while (!e.isNull()) {
        if ((attributeName = e.attribute("name")) == "nTransfers") {
            numTransfers = e.text().toUShort();
        } else {
            QRegExp rx("curve(\\d+)");

            if (rx.indexIn(attributeName, 0) != -1) {

                index = rx.cap(1).toUShort();
                index = qMin(index, quint16(curves.count()));

                if (!e.text().isEmpty()) {
                    curve = KisCubicCurve(e.text());
                }
                curves.insert(index, curve);
            }
        }
        e = e.nextSiblingElement();
    }

    //prepend empty curves for the brightness contrast filter.
    if(getString("legacy") == "brightnesscontrast") {
        if (getString("colorModel") == LABAColorModelID.id()) {
            curves.append(KisCubicCurve());
            curves.append(KisCubicCurve());
            curves.append(KisCubicCurve());
        } else {
            int extraChannels = 5;
            if (getString("colorModel") == CMYKAColorModelID.id()) {
                extraChannels = 6;
            } else if (getString("colorModel") == GrayAColorModelID.id()) {
                extraChannels = 0;
            }
            for(int c = 0; c < extraChannels; c ++) {
                curves.insert(0, KisCubicCurve());
            }
        }
    }
    if (!numTransfers)
        return;

    setVersion(version);
    setCurves(curves);
}

/**
 * Inherited from KisPropertiesConfiguration
 */
//void KisMultiChannelFilterConfiguration::fromXML(const QString& s)

void addParamNode(QDomDocument& doc,
                  QDomElement& root,
                  const QString &name,
                  const QString &value)
{
    QDomText text = doc.createTextNode(value);
    QDomElement t = doc.createElement("param");
    t.setAttribute("name", name);
    t.appendChild(text);
    root.appendChild(t);
}

void KisMultiChannelFilterConfiguration::toXML(QDomDocument& doc, QDomElement& root) const
{
    /**
     * @code
     * <params version=1>
     *       <param name="nTransfers">3</param>
     *       <param name="curve0">0,0;0.5,0.5;1,1;</param>
     *       <param name="curve1">0,0;1,1;</param>
     *       <param name="curve2">0,0;1,1;</param>
     * </params>
     * @endcode
     */

    root.setAttribute("version", version());

    QDomText text;
    QDomElement t;

    addParamNode(doc, root, "nTransfers", QString::number(m_channelCount));

    KisCubicCurve curve;
    QString paramName;

    for (int i = 0; i < m_curves.size(); ++i) {
        QString name = QLatin1String("curve") + QString::number(i);
        QString value = m_curves[i].toString();

        addParamNode(doc, root, name, value);
    }
}

bool KisMultiChannelFilterConfiguration::compareTo(const KisPropertiesConfiguration *rhs) const
{
    const KisMultiChannelFilterConfiguration *otherConfig = dynamic_cast<const KisMultiChannelFilterConfiguration *>(rhs);

    return otherConfig
        && KisFilterConfiguration::compareTo(rhs)
        && m_channelCount == otherConfig->m_channelCount
        && m_curves == otherConfig->m_curves
        && m_transfers == otherConfig->m_transfers;
}

bool KisMultiChannelFilterConfiguration::hasProperty(const QString& name) const
{
    if (KisColorTransformationConfiguration::hasProperty(name)) {
        return true;
    }
    if (name == "nTransfers") {
        return true;
    }
    int curveIndex;
    if (!curveIndexFromCurvePropertyName(name, curveIndex)) {
        return false;
    }
    return curveIndex >= 0 && curveIndex < m_channelCount;
}

void KisMultiChannelFilterConfiguration::setProperty(const QString& name, const QVariant& value)
{
    if (name == "nTransfers") {
        // No Op
        return;
    }
    int curveIndex;
    if (!curveIndexFromCurvePropertyName(name, curveIndex) ||
        curveIndex < 0 || curveIndex >= m_channelCount) {
        KisColorTransformationConfiguration::setProperty(name, value);
        return;
    }
    KIS_SAFE_ASSERT_RECOVER_RETURN(value.canConvert<QString>());
    m_curves[curveIndex] = KisCubicCurve(value.toString());
    updateTransfer(curveIndex);
    invalidateColorTransformationCache();
}

bool KisMultiChannelFilterConfiguration::getProperty(const QString& name, QVariant& value) const
{
    if (KisColorTransformationConfiguration::hasProperty(name)) {
        return KisColorTransformationConfiguration::getProperty(name, value);
    }
    if (name == "nTransfers") {
        value = m_curves.size();
        return true;
    }
    int curveIndex;
    if (!curveIndexFromCurvePropertyName(name, curveIndex) ||
        curveIndex < 0 || curveIndex >= m_channelCount) {
        return false;
    }
    value = m_curves[curveIndex].toString();
    return true;
}

QVariant KisMultiChannelFilterConfiguration::getProperty(const QString& name) const
{
    if (KisColorTransformationConfiguration::hasProperty(name)) {
        return KisColorTransformationConfiguration::getProperty(name);
    }
    if (name == "nTransfers") {
        return m_curves.size();
    }
    int curveIndex;
    if (!curveIndexFromCurvePropertyName(name, curveIndex) ||
        curveIndex < 0 || curveIndex >= m_channelCount) {
        return false;
    }
    return m_curves[curveIndex].toString();
}

QMap<QString, QVariant> KisMultiChannelFilterConfiguration::getProperties() const
{
    QMap<QString, QVariant> propertyMap = KisColorTransformationConfiguration::getProperties();
    propertyMap.insert("nTransfers", m_curves.size());
    for (int i = 0; i < m_curves.size(); ++i) {
        const QString name = QLatin1String("curve") + QString::number(i);
        const QString value = m_curves[i].toString();
        propertyMap.insert(name, value);
    }
    return propertyMap;
}

bool KisMultiChannelFilterConfiguration::curveIndexFromCurvePropertyName(const QString& name, int& curveIndex) const
{
    QRegExp rx("curve(\\d+)");
    if (rx.indexIn(name, 0) == -1) {
        return false;
    }

    curveIndex = rx.cap(1).toUShort();
    return true;
}

KisMultiChannelConfigWidget::KisMultiChannelConfigWidget(QWidget * parent, KisPaintDeviceSP dev, Qt::WindowFlags f)
        : KisConfigWidget(parent, f)
        , m_dev(dev)
        , m_page(new WdgPerChannel(this))
        , m_hslHistogram(nullptr)
        , m_channelsHistogram(nullptr)
{
    Q_ASSERT(m_dev);

    const KoColorSpace *targetColorSpace = dev->compositionSourceColorSpace();
    m_virtualChannels = KisMultiChannelFilter::getVirtualChannels(targetColorSpace);
}

/**
 * Initialize the dialog.
 * Note: m_virtualChannels must be populated before calling this
 */
void KisMultiChannelConfigWidget::init() {
    QHBoxLayout * layout = new QHBoxLayout(this);
    Q_CHECK_PTR(layout);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_page);
    setButtonsIcons();

    resetCurves();

    const int virtualChannelCount = m_virtualChannels.size();
    for (int i = 0; i < virtualChannelCount; i++) {
        const VirtualChannelInfo &info = m_virtualChannels[i];
        m_page->cmbChannel->addItem(info.name(), i);
    }

    connect(m_page->cmbChannel, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index){
        slotChannelSelected(index);
        setActiveChannel(true, m_page->cmbDriverChannel->isHidden());
    });

    connect(m_page->buttonGroupHistogramMode, SIGNAL(buttonToggled(QAbstractButton*, bool)), SLOT(slot_buttonGroupHistogramMode_buttonToggled(QAbstractButton*, bool)));

    connect(m_page->buttonScaleHistogramToFit, &QToolButton::clicked, [=](){
        m_histogramPainter.setScaleToFit();
        m_page->curveWidget->setPixmap(getHistogram());
    });

    connect(m_page->buttonScaleHistogramToCutLongPeaks, &QToolButton::clicked, [=](){
        m_histogramPainter.setScaleToCutLongPeaks();
        m_page->curveWidget->setPixmap(getHistogram());
    });

    connect(m_page->buttonReset, SIGNAL(clicked()), this, SLOT(resetCurve()));

    initHistogramCalculator();

    connect(m_page->curveWidget, SIGNAL(modified()), this, SIGNAL(sigConfigurationItemChanged()));

    {
        KisSignalsBlocker b(m_page->curveWidget);
        m_page->curveWidget->setCurve(m_curves[0]);
        slotChannelSelected(0);
    }

    setActiveChannel();
}

KisMultiChannelConfigWidget::~KisMultiChannelConfigWidget()
{
}

void KisMultiChannelConfigWidget::resetCurves()
{
    const KisPropertiesConfigurationSP &defaultConfiguration = getDefaultConfiguration();
    const auto *defaults = dynamic_cast<const KisMultiChannelFilterConfiguration*>(defaultConfiguration.data());

    KIS_SAFE_ASSERT_RECOVER_RETURN(defaults);
    m_curves = defaults->curves();

    const int virtualChannelCount = m_virtualChannels.size();
    for (int i = 0; i < virtualChannelCount; i++) {
        const VirtualChannelInfo &info = m_virtualChannels[i];
        m_curves[i].setName(info.name());
    }
}

void KisMultiChannelConfigWidget::setConfiguration(const KisPropertiesConfigurationSP config)
{
    const KisMultiChannelFilterConfiguration * cfg = dynamic_cast<const KisMultiChannelFilterConfiguration *>(config.data());
    if (!cfg) {
        return;
    }

    if (cfg->curves().empty()) {
        /**
         * HACK ALERT: our configuration factory generates
         * default configuration with nTransfers==0.
         * Catching it here. Just set everything to defaults instead.
         */
        const KisPropertiesConfigurationSP &defaultConfiguration = getDefaultConfiguration();
        const auto *defaults = dynamic_cast<const KisMultiChannelFilterConfiguration*>(defaultConfiguration.data());
        KIS_SAFE_ASSERT_RECOVER_RETURN(defaults);

        if (!defaults->curves().isEmpty()) {
            setConfiguration(defaultConfiguration);
            return;
        }
    } else if (cfg->curves().size() > m_virtualChannels.size()) {
        QMessageBox::warning(this, i18nc("@title:window", "Krita"), i18n("The current configuration was created for a different colorspace and cannot be used. All curves will be reset."));
        warnKrita << "WARNING: trying to load a curve with invalid number of channels!";
        warnKrita << "WARNING:   expected:" << m_virtualChannels.size();
        warnKrita << "WARNING:        got:" << cfg->curves().size();
        return;
    } else {
        if (cfg->curves().size() < m_virtualChannels.size()) {
            // The configuration does not cover all our channels.
            // This happens when loading a document from an older version, which supported fewer channels.
            // Reset to make sure the unspecified channels have their default values.
            resetCurves();
        }

        for (int ch = 0; ch < cfg->curves().size(); ch++) {
            m_curves[ch] = cfg->curves()[ch];
        }
    }

    // HACK: we save the previous curve in setActiveChannel, so just copy it
    m_page->curveWidget->setCurve(m_curves[m_activeVChannel]);

    slotChannelSelected(0);
    setActiveChannel();
}

inline QPixmap KisMultiChannelConfigWidget::createGradient(Qt::Orientation orient /*, int invert (not used yet) */)
{
    int width;
    int height;
    int *i, inc, col;
    int x = 0, y = 0;

    const VirtualChannelInfo &info = (!m_page->cmbDriverChannel->isHidden() && orient == Qt::Horizontal)
        ? m_virtualChannels[m_activeVDriverChannel] : m_virtualChannels[m_activeVChannel];
    const bool isRealChannel = (info.type() == VirtualChannelInfo::ALL_COLORS || info.type() == VirtualChannelInfo::REAL);

    const int maxsize = isRealChannel
        ? m_channelsHistogram->producer()->numberOfBins(m_histogramPainter.channels().at(0))
        : m_hslHistogram->producer()->numberOfBins(m_histogramPainter.channels().at(0));

    if (orient == Qt::Horizontal) {
        i = &x; inc = 1; col = 0;
        width = maxsize; height = 1;
    } else {
        i = &y; inc = -1; col = maxsize - 1;
        width = 1; height = maxsize;
    }

    QPixmap gradientpix(width, height);
    QPainter p(&gradientpix);
    p.setPen(QPen(QColor(0, 0, 0), 1, Qt::SolidLine));

    if (isRealChannel) {
        QColor leftColor, rightColor;
        const int channelIndex = info.pixelIndex();
        const KoColorSpace *colorSpace = m_dev->compositionSourceColorSpace();
        if (colorSpace->colorModelId() == GrayAColorModelID || info.type() == VirtualChannelInfo::ALL_COLORS || info.isAlpha()) {
            if (info.type() != VirtualChannelInfo::ALL_COLORS) {
                colorSpace = KoColorSpaceRegistry::instance()->graya8();
            }

            leftColor = Qt::black;
            rightColor = Qt::white;
        } else if (colorSpace->colorModelId() == RGBAColorModelID && colorSpace->colorDepthId().id().contains("U")) {
            leftColor = Qt::black;
            rightColor = channelIndex == 0 ? Qt::blue : (channelIndex == 1 ? Qt::green : Qt::red);
        } else if (colorSpace->colorModelId() == RGBAColorModelID && colorSpace->colorDepthId().id().contains("F")) {
            leftColor = Qt::black;
            rightColor = channelIndex == 0 ? Qt::red : (channelIndex == 1 ? Qt::green : Qt::blue);
        } else if (colorSpace->colorModelId() == CMYKAColorModelID) {
            leftColor = Qt::white;
            rightColor = channelIndex == 0 ? Qt::cyan : (channelIndex == 1 ? Qt::magenta : (channelIndex == 2 ? Qt::yellow : Qt::black));
        } else {
            leftColor = Qt::black;
            rightColor = Qt::white;
        }

        int weightSum = maxsize - 1;
        KoColor leftKoColor(leftColor, colorSpace);
        KoColor rightKoColor(rightColor, colorSpace);
        const quint8 *colors[2];
        colors[0] = leftKoColor.data();
        colors[1] = rightKoColor.data();
        qint16 weights[2];
        KoColor penColor(colorSpace);

        for (; *i < maxsize; (*i)++, col += inc) {
            weights[0] = weightSum - col;
            weights[1] = col;

            colorSpace->mixColorsOp()->mixColors(colors, weights, 2, penColor.data(), weightSum);
            p.setPen(penColor.toQColor());
            p.drawPoint(x, y);
        }
    } else {
        const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb16();
        QVector<qreal> lumaCoeff = colorSpace->lumaCoefficients();
        KoColor penColor(colorSpace);
        quint16* rgba = reinterpret_cast<quint16*>(penColor.data());
        qreal hue, sat, luma, r, g, b;

        for (; *i < maxsize; (*i)++, col += inc) {
            qreal t = col / qreal(maxsize - 1);
            if (info.type() == VirtualChannelInfo::HUE) {
                hue = t;
                sat = 1.0;
                luma = pow(0.5, 2.2);
            } else if (info.type() == VirtualChannelInfo::SATURATION) {
                hue = 0.0;
                sat = t;
                luma = pow(0.5, 2.2);
            } else {
                hue = 0.0;
                sat = 0.0;
                luma = pow(t, 2.2);
            }

            HSYToRGB(hue, sat, luma, &r, &g, &b, lumaCoeff[0], lumaCoeff[1], lumaCoeff[2]);

            rgba[0] = KoColorSpaceMaths<qreal, quint16>::scaleToA(b);
            rgba[1] = KoColorSpaceMaths<qreal, quint16>::scaleToA(g);
            rgba[2] = KoColorSpaceMaths<qreal, quint16>::scaleToA(r);
            rgba[3] = KoColorSpaceMaths<qreal, quint16>::scaleToA(1.0);

            p.setPen(penColor.toQColor());
            p.drawPoint(x, y);
        }
    }

    return gradientpix;
}

inline QPixmap KisMultiChannelConfigWidget::getHistogram()
{
    int height = 256;
    QPixmap pix(256, height);

    QPalette appPalette = QApplication::palette();

    pix.fill(QColor(appPalette.color(QPalette::Base)));
    QPainter painter(&pix);
    painter.save();

    const VirtualChannelInfo &info = m_page->cmbDriverChannel->isHidden() ? m_virtualChannels[m_activeVChannel] : m_virtualChannels[m_activeVDriverChannel];
    if (info.type() == VirtualChannelInfo::ALL_COLORS) {
        QVector<int> channels;
        for (const VirtualChannelInfo &channelInfo : m_virtualChannels) {
            if (channelInfo.type() == VirtualChannelInfo::REAL && !channelInfo.isAlpha()) {
                channels.append(channelInfo.pixelIndex());
            }
        }

        m_histogramPainter.setChannels(channels);
    } else if (info.type() == VirtualChannelInfo::REAL) {
        m_histogramPainter.setChannel(info.pixelIndex());
    } else {
        if (info.type() == VirtualChannelInfo::HUE) {
            m_histogramPainter.setChannel(0);
        } else if (info.type() == VirtualChannelInfo::SATURATION) {
            m_histogramPainter.setChannel(1);
        } else {
            m_histogramPainter.setChannel(2);
        } 
    }

    QImage histogramImage = m_histogramPainter.paint(256, height);

    QLinearGradient shadowGradient(QPointF(0.0, 0.0), QPointF(0.0, static_cast<qreal>(height) * 0.2));
    shadowGradient.setColorAt(0.00, QColor(0, 0, 0, 64));
    shadowGradient.setColorAt(0.25, QColor(0, 0, 0, 36));
    shadowGradient.setColorAt(0.50, QColor(0, 0, 0, 16));
    shadowGradient.setColorAt(0.75, QColor(0, 0, 0, 4));
    shadowGradient.setColorAt(1.00, QColor(0, 0, 0, 0));

    QPainter histogramPainter(&histogramImage);
    histogramPainter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    histogramPainter.fillRect(histogramImage.rect(), shadowGradient);

    painter.drawImage(0, 0, histogramImage);
    painter.setOpacity(1.0);

    painter.restore();
    return pix;
}

void KisMultiChannelConfigWidget::slotChannelSelected(int index)
{
    const int virtualChannel = m_page->cmbChannel->itemData(index).toInt();

    m_curves[m_activeVChannel] = m_page->curveWidget->curve();
    m_activeVChannel = virtualChannel;
    m_page->curveWidget->setCurve(m_curves[m_activeVChannel]);

    int currentIndex = m_page->cmbChannel->findData(m_activeVChannel);
    m_page->cmbChannel->setCurrentIndex(currentIndex);
}

void KisMultiChannelConfigWidget::setActiveChannel(bool setVgradient, bool setHgradientAndHistogram)
{
    if (setHgradientAndHistogram) {
        initHistogramCalculator();
        m_page->curveWidget->setPixmap(getHistogram());

        m_page->hgradient->setPixmap(createGradient(Qt::Horizontal));
    }

    if (setVgradient) {
        m_page->vgradient->setPixmap(createGradient(Qt::Vertical));
    }

    updateChannelControls();
}

void KisMultiChannelConfigWidget::initHistogramCalculator()
{
    const KoColorSpace *targetColorSpace = m_dev->compositionSourceColorSpace();
    const VirtualChannelInfo &info = m_page->cmbDriverChannel->isHidden() ? m_virtualChannels[m_activeVChannel] : m_virtualChannels[m_activeVDriverChannel];

    if (info.type() == VirtualChannelInfo::ALL_COLORS || info.type() == VirtualChannelInfo::REAL) {
        if (!m_channelsHistogram) {
            QList<QString> keys = KoHistogramProducerFactoryRegistry::instance()->keysCompatibleWith(targetColorSpace);

            if (keys.size() > 0) {
                KoHistogramProducerFactory *hpf = KoHistogramProducerFactoryRegistry::instance()->get(keys.at(0));
                m_channelsHistogram.reset(new KisHistogram(m_dev, m_dev->exactBounds(), hpf->generate(), LINEAR));
            }
        }

        m_histogramPainter.setup(m_channelsHistogram.data(), targetColorSpace);
    } else {
        if (!m_hslHistogram) {
            KoHistogramProducer *HSLHistogramProducer = new KoGenericHSLHistogramProducer();
            m_hslHistogram.reset(new KisHistogram(m_dev, m_dev->exactBounds(), HSLHistogramProducer, LINEAR));
        }

        m_histogramPainter.setup(m_hslHistogram.data(), 0, QVector<int>({0, 1, 2}));
    }
}

void KisMultiChannelConfigWidget::slot_buttonGroupHistogramMode_buttonToggled(QAbstractButton *button, bool checked)
{
    if (!checked) {
        return;
    }

    m_histogramPainter.setLogarithmic(button == m_page->buttonLogarithmicHistogram);
    m_page->curveWidget->setPixmap(getHistogram());
}

void KisMultiChannelConfigWidget::resetCurve()
{
    const KisPropertiesConfigurationSP &defaultConfiguration = getDefaultConfiguration();
    const auto *defaults = dynamic_cast<const KisMultiChannelFilterConfiguration*>(defaultConfiguration.data());
    KIS_SAFE_ASSERT_RECOVER_RETURN(defaults);

    auto defaultCurves = defaults->curves();
    KIS_SAFE_ASSERT_RECOVER_RETURN(defaultCurves.size() > m_activeVChannel);

    m_page->curveWidget->setCurve(defaultCurves[m_activeVChannel]);
}

void KisMultiChannelConfigWidget::setButtonsIcons()
{
    m_page->buttonLinearHistogram->setIcon(KisIconUtils::loadIcon("histogram-linear"));
    m_page->buttonLogarithmicHistogram->setIcon(KisIconUtils::loadIcon("histogram-logarithmic"));
    m_page->buttonScaleHistogramToFit->setIcon(KisIconUtils::loadIcon("histogram-show-all"));
    m_page->buttonScaleHistogramToCutLongPeaks->setIcon(KisIconUtils::loadIcon("histogram-show-best"));
}

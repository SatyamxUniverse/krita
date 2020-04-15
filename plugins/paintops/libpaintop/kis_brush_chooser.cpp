/*
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2009 Sven Langkamp <sven.langkamp@gmail.com>
 *  Copyright (c) 2010 Cyrille Berger <cberger@cberger.net>
 *  Copyright (c) 2010 Lukáš Tvrdý <lukast.dev@gmail.com>
 *  Copyright (C) 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_brush_chooser.h"

#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QAbstractItemDelegate>
#include <klocalizedstring.h>

#include <kconfig.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>
#include <KisKineticScroller.h>

#include <KisResourceItemView.h>
#include <KisResourceItemChooser.h>
#include <KisResourceModel.h>

#include <kis_icon.h>
#include "KisBrushServerProvider.h"
#include "widgets/kis_slider_spin_box.h"
#include "widgets/kis_multipliers_double_slider_spinbox.h"
#include "kis_spacing_selection_widget.h"
#include "kis_signals_blocker.h"

#include "kis_imagepipe_brush.h"
#include "kis_custom_brush_widget.h"
#include "kis_clipboard_brush_widget.h"
#include <kis_config.h>

#include "kis_global.h"
#include "kis_gbr_brush.h"
#include "kis_png_brush.h"
#include "kis_debug.h"
#include "kis_image.h"
#include <KisGlobalResourcesInterface.h>

/// The resource item delegate for rendering the resource preview
class KisBrushDelegate : public QAbstractItemDelegate
{
public:
    KisBrushDelegate(QObject * parent = 0) : QAbstractItemDelegate(parent) {}
    ~KisBrushDelegate() override {}
    /// reimplemented
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    /// reimplemented
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex &) const override {
        return option.decorationSize;
    }
};

void KisBrushDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (! index.isValid())
        return;

    QImage thumbnail = index.data(Qt::UserRole + KisResourceModel::Thumbnail).value<QImage>();

    QRect itemRect = option.rect;

    if (thumbnail.height() > itemRect.height() || thumbnail.width() > itemRect.width()) {
        thumbnail = thumbnail.scaled(itemRect.size() , Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    painter->save();
    int dx = (itemRect.width() - thumbnail.width()) / 2;
    int dy = (itemRect.height() - thumbnail.height()) / 2;
    painter->drawImage(itemRect.x() + dx, itemRect.y() + dy, thumbnail);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlight(), 2.0));
        painter->drawRect(option.rect);
        painter->setCompositionMode(QPainter::CompositionMode_HardLight);
        painter->setOpacity(0.65);
        painter->fillRect(option.rect, option.palette.highlight());
    }

    painter->restore();
}


KisPredefinedBrushChooser::KisPredefinedBrushChooser(QWidget *parent, const char *name)
    : QWidget(parent),
      m_stampBrushWidget(0),
      m_clipboardBrushWidget(0)
{
    setObjectName(name);

    setupUi(this);

    brushSizeSpinBox->setRange(0, KSharedConfig::openConfig()->group("").readEntry("maximumBrushSize", 1000), 2);
    brushSizeSpinBox->setValue(5);
    brushSizeSpinBox->setExponentRatio(3.0);
    brushSizeSpinBox->setSuffix(i18n(" px"));
    brushSizeSpinBox->setExponentRatio(3.0);

    QObject::connect(brushSizeSpinBox, SIGNAL(valueChanged(qreal)), this, SLOT(slotSetItemSize(qreal)));

    brushRotationSpinBox->setRange(0, 360, 0);
    brushRotationSpinBox->setValue(0);
    brushRotationSpinBox->setSuffix(QChar(Qt::Key_degree));
    QObject::connect(brushRotationSpinBox, SIGNAL(valueChanged(qreal)), this, SLOT(slotSetItemRotation(qreal)));

    brushSpacingSelectionWidget->setSpacing(true, 1.0);
    connect(brushSpacingSelectionWidget, SIGNAL(sigSpacingChanged()), SLOT(slotSpacingChanged()));

    m_itemChooser = new KisResourceItemChooser(ResourceType::Brushes, false, this);
    m_itemChooser->setObjectName("brush_selector");

    m_itemChooser->showTaggingBar(true);
    m_itemChooser->setRowHeight(30);
    m_itemChooser->setItemDelegate(new KisBrushDelegate(this));
    m_itemChooser->setCurrentItem(0);
    m_itemChooser->setSynced(true);
    m_itemChooser->setMinimumWidth(100);
    m_itemChooser->setMinimumHeight(150);
    m_itemChooser->showButtons(false); // turn the import and delete buttons since we want control over them


    addPresetButton->setIcon(KisIconUtils::loadIcon("list-add"));
    deleteBrushTipButton->setIcon(KisIconUtils::loadIcon("trash-empty"));



    connect(addPresetButton, SIGNAL(clicked(bool)), this, SLOT(slotImportNewBrushResource()));
    connect(deleteBrushTipButton, SIGNAL(clicked(bool)), this, SLOT(slotDeleteBrushResource()));

    presetsLayout->addWidget(m_itemChooser);


    connect(m_itemChooser, SIGNAL(resourceSelected(KoResourceSP )), this, SLOT(updateBrushTip(KoResourceSP )));

    stampButton->setIcon(KisIconUtils::loadIcon("list-add"));
    stampButton->setToolTip(i18n("Creates a brush tip from the current image selection."
                               "\n If no selection is present the whole image will be used."));

    clipboardButton->setIcon(KisIconUtils::loadIcon("list-add"));
    clipboardButton->setToolTip(i18n("Creates a brush tip from the image in the clipboard."));

    connect(stampButton, SIGNAL(clicked()), this,  SLOT(slotOpenStampBrush()));
    connect(clipboardButton, SIGNAL(clicked()), SLOT(slotOpenClipboardBrush()));

    QGridLayout *spacingLayout = new QGridLayout();
    spacingLayout->setObjectName("spacing grid layout");

    resetBrushButton->setToolTip(i18n("Reloads Spacing from file\nSets Scale to 1.0\nSets Rotation to 0.0"));
    connect(resetBrushButton, SIGNAL(clicked()), SLOT(slotResetBrush()));

    intAdjustmentMidPoint->setRange(0, 255);
    intAdjustmentMidPoint->setPageStep(10);
    intAdjustmentMidPoint->setSingleStep(1);
    intAdjustmentMidPoint->setPrefix(i18nc("@label:slider", "Neutral point: "));

    intBrightnessAdjustment->setRange(-100, 100);
    intBrightnessAdjustment->setPageStep(10);
    intBrightnessAdjustment->setSingleStep(1);
    intBrightnessAdjustment->setSuffix("%");
    intBrightnessAdjustment->setPrefix(i18nc("@label:slider", "Brightness: "));

    intContrastAdjustment->setRange(-100, 100);
    intContrastAdjustment->setPageStep(10);
    intContrastAdjustment->setSingleStep(1);
    intContrastAdjustment->setSuffix("%");
    intContrastAdjustment->setPrefix(i18nc("@label:slider", "Contrast: "));

    btnResetAdjustments->setToolTip(i18nc("@info:tooltip", "Resets all the adjustments to default values:\n Neutral Point: 127\n Brightness: 0%\n Contrast: 0%"));
    connect(btnResetAdjustments, SIGNAL(clicked()), SLOT(slotResetAdjustments()));

    connect(btnMaskMode, SIGNAL(toggled(bool)), SLOT(slotUpdateBrushAdjustmentsState()));
    connect(btnColorMode, SIGNAL(toggled(bool)), SLOT(slotUpdateBrushAdjustmentsState()));
    connect(btnLightnessMode, SIGNAL(toggled(bool)), SLOT(slotUpdateBrushAdjustmentsState()));

    connect(btnMaskMode, SIGNAL(toggled(bool)), SLOT(slotWriteBrushMode()));
    connect(btnColorMode, SIGNAL(toggled(bool)), SLOT(slotWriteBrushMode()));
    connect(btnLightnessMode, SIGNAL(toggled(bool)), SLOT(slotWriteBrushMode()));

    connect(btnMaskMode, SIGNAL(toggled(bool)), SLOT(slotUpdateResetBrushAdjustmentsButtonState()));
    connect(btnColorMode, SIGNAL(toggled(bool)), SLOT(slotUpdateResetBrushAdjustmentsButtonState()));
    connect(btnLightnessMode, SIGNAL(toggled(bool)), SLOT(slotUpdateResetBrushAdjustmentsButtonState()));

    connect(intAdjustmentMidPoint, SIGNAL(valueChanged(int)), SLOT(slotWriteBrushAdjustments()));
    connect(intBrightnessAdjustment, SIGNAL(valueChanged(int)), SLOT(slotWriteBrushAdjustments()));
    connect(intContrastAdjustment, SIGNAL(valueChanged(int)), SLOT(slotWriteBrushAdjustments()));

    connect(intAdjustmentMidPoint, SIGNAL(valueChanged(int)), SLOT(slotUpdateResetBrushAdjustmentsButtonState()));
    connect(intBrightnessAdjustment, SIGNAL(valueChanged(int)), SLOT(slotUpdateResetBrushAdjustmentsButtonState()));
    connect(intContrastAdjustment, SIGNAL(valueChanged(int)), SLOT(slotUpdateResetBrushAdjustmentsButtonState()));

    updateBrushTip(m_itemChooser->currentResource());
}

KisPredefinedBrushChooser::~KisPredefinedBrushChooser()
{
}

void KisPredefinedBrushChooser::setBrush(KisBrushSP brush)
{
    /**
     * Warning: since the brushes are always cloned after loading from XML or
     * fetching from the server, we cannot just ask for that brush explicitly.
     * Instead, we should search for the brush with the same filename and/or name
     * and load it. Please take it into account that after selecting the brush
     * explicitly in the chooser, m_itemChooser->currentResource() might be
     * **not** the same as the value in m_brush.
     *
     * Ideally, if the resource is not found on the server, we should add it, but
     * it might lead to a set of weird consequences. So for now we just
     * select nothing.
     */

    KoResourceServer<KisBrush>* server = KisBrushServerProvider::instance()->brushServer();
    KoResourceSP resource = server->resourceByFilename(brush->filename());

    if (!resource) {
        resource = server->resourceByName(brush->name());
    }

    if (!resource) {
        resource = brush;
    }

    m_itemChooser->setCurrentResource(resource);
    updateBrushTip(brush, true);
}

void KisPredefinedBrushChooser::slotResetBrush()
{
    /**
     * The slot also resets the brush on the server
     *
     * TODO: technically, after we refactored all the brushes to be forked,
     *       we can just re-update the brush from the server without reloading.
     *       But it needs testing.
     */

    KisBrushSP brush = m_itemChooser->currentResource().dynamicCast<KisBrush>();
    if (brush) {
        brush->load(KisGlobalResourcesInterface::instance());
        brush->setScale(1.0);
        brush->setAngle(0.0);

        if (KisColorfulBrush *colorfulBrush = dynamic_cast<KisColorfulBrush*>(m_brush.data())) {
            colorfulBrush->setUseColorAsMask(false);
            colorfulBrush->setPreserveLightness(false);
            colorfulBrush->setAdjustmentMidPoint(127);
            colorfulBrush->setBrightnessAdjustment(0.0);
            colorfulBrush->setContrastAdjustment(0.0);
        }

        updateBrushTip(brush);
        emit sigBrushChanged();
    }
}

void KisPredefinedBrushChooser::slotSetItemSize(qreal sizeValue)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_brush);

    if (m_brush) {
        int brushWidth = m_brush->width();

        m_brush->setScale(sizeValue / qreal(brushWidth));
        emit sigBrushChanged();
    }
}

void KisPredefinedBrushChooser::slotSetItemRotation(qreal rotationValue)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_brush);

    if (m_brush) {
        m_brush->setAngle(rotationValue / 180.0 * M_PI);
        emit sigBrushChanged();
    }
}

void KisPredefinedBrushChooser::slotSpacingChanged()
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_brush);

    if (m_brush) {
        m_brush->setSpacing(brushSpacingSelectionWidget->spacing());
        m_brush->setAutoSpacing(brushSpacingSelectionWidget->autoSpacingActive(), brushSpacingSelectionWidget->autoSpacingCoeff());

        emit sigBrushChanged();
    }
}

void KisPredefinedBrushChooser::slotOpenStampBrush()
{
    if(!m_stampBrushWidget) {
        m_stampBrushWidget = new KisCustomBrushWidget(this, i18n("Stamp"), m_image);
        m_stampBrushWidget->setModal(false);
        connect(m_stampBrushWidget, SIGNAL(sigNewPredefinedBrush(KoResourceSP )),
                                    SLOT(slotNewPredefinedBrush(KoResourceSP )));
    } else {
        m_stampBrushWidget->setImage(m_image);
    }

    QDialog::DialogCode result = (QDialog::DialogCode)m_stampBrushWidget->exec();

    if(result) {
        updateBrushTip(m_itemChooser->currentResource());
    }
}
void KisPredefinedBrushChooser::slotOpenClipboardBrush()
{
    if(!m_clipboardBrushWidget) {
        m_clipboardBrushWidget = new KisClipboardBrushWidget(this, i18n("Clipboard"), m_image);
        m_clipboardBrushWidget->setModal(true);
        connect(m_clipboardBrushWidget, SIGNAL(sigNewPredefinedBrush(KoResourceSP )),
                                        SLOT(slotNewPredefinedBrush(KoResourceSP )));
    }

    QDialog::DialogCode result = (QDialog::DialogCode)m_clipboardBrushWidget->exec();

    if(result) {
        updateBrushTip(m_itemChooser->currentResource());
    }
}

void KisPredefinedBrushChooser::updateBrushTip(KoResourceSP resource, bool isChangingBrushPresets)
{


    QString animatedBrushTipSelectionMode; // incremental, random, etc

    {
        KisBrushSP brush = resource.dynamicCast<KisBrush>();
        m_brush = brush ? brush->clone().dynamicCast<KisBrush>() : 0;
    }

    if (m_brush) {
        brushTipNameLabel->setText(i18n(m_brush->name().toUtf8().data()));

        QString brushTypeString = "";

        if (m_brush->brushType() == INVALID) {
            brushTypeString = i18n("Invalid");
        } else if (m_brush->brushType() == MASK) {
            brushTypeString = i18n("Mask");
        } else if (m_brush->brushType() == IMAGE) {
            brushTypeString = i18n("GBR");
        } else if (m_brush->brushType() == PIPE_MASK ) {
            brushTypeString = i18n("Animated Mask"); // GIH brush

            // cast to GIH brush and grab parasite name
            //m_brush
            KisImagePipeBrushSP pipeBrush = resource.dynamicCast<KisImagePipeBrush>();
            animatedBrushTipSelectionMode =  pipeBrush->parasiteSelection();


        } else if (m_brush->brushType() == PIPE_IMAGE ) {
            brushTypeString = i18n("Animated Image");
        }

        QString brushDetailsText = QString("%1 (%2 x %3) %4")
                       .arg(brushTypeString)
                       .arg(m_brush->width())
                       .arg(m_brush->height())
                       .arg(animatedBrushTipSelectionMode);

        brushDetailsLabel->setText(brushDetailsText);

        // keep the current preset's tip settings if we are preserving it
        // this will set the brush's model data to keep what it currently has for size, spacing, etc.
        if (preserveBrushPresetSettings->isChecked() && !isChangingBrushPresets) {
            m_brush->setAutoSpacing(brushSpacingSelectionWidget->autoSpacingActive(), brushSpacingSelectionWidget->autoSpacingCoeff());
            m_brush->setAngle(brushRotationSpinBox->value() * M_PI / 180);
            m_brush->setSpacing(brushSpacingSelectionWidget->spacing());
            m_brush->setUserEffectiveSize(brushSizeSpinBox->value());
        }

        brushSpacingSelectionWidget->setSpacing(m_brush->autoSpacingActive(),
                                m_brush->autoSpacingActive() ?
                                m_brush->autoSpacingCoeff() : m_brush->spacing());

        brushRotationSpinBox->setValue(m_brush->angle() * 180 / M_PI);
        brushSizeSpinBox->setValue(m_brush->width() * m_brush->scale());

        emit sigBrushChanged();
    }

    slotUpdateBrushModeButtonsState();
}

#include "kis_scaling_size_brush.h"

void KisPredefinedBrushChooser::slotUpdateBrushModeButtonsState()
{
    KisColorfulBrush *colorfulBrush = dynamic_cast<KisColorfulBrush*>(m_brush.data());
    const bool modeSwitchEnabled =
        m_hslBrushTipEnabled && colorfulBrush && colorfulBrush->hasColor();

    if (modeSwitchEnabled) {
        if (colorfulBrush->useColorAsMask() && colorfulBrush->preserveLightness()) {
            btnLightnessMode->setChecked(true);
        } else if (colorfulBrush->useColorAsMask()) {
            btnMaskMode->setChecked(true);
        } else {
            btnColorMode->setChecked(true);
        }

        {
            // sliders emit update signals when modified from the code
            KisSignalsBlocker b(intAdjustmentMidPoint, intBrightnessAdjustment, intContrastAdjustment);
            intAdjustmentMidPoint->setValue(colorfulBrush->adjustmentMidPoint());
            intBrightnessAdjustment->setValue(qRound(colorfulBrush->brightnessAdjustment() * 100.0));
            intContrastAdjustment->setValue(qRound(colorfulBrush->contrastAdjustment() * 100.0));
        }

        btnMaskMode->setToolTip("Luminosity of the brush tip image is used as alpha channel for the stroke");
        btnColorMode->setToolTip("The brush tip image is painted as it is");
        btnLightnessMode->setToolTip("Luminosity of the brush tip image is used as lightness correction for the painting color. Alpha channel of the brush tip image is used as alpha for the final stroke");
        intAdjustmentMidPoint->setToolTip("Luminosity value of the brush that will not change the painting color. All brush pixels darker than neutral point will paint with darker color, pixels lighter than neutral point — lighter.");
        intBrightnessAdjustment->setToolTip("Brightness correction for the brush");
        intContrastAdjustment->setToolTip("Contrast correction for the brush");
        grpBrushMode->setToolTip("");
    } else {

        {
            // sliders emit update signals when modified from the code
            KisSignalsBlocker b(intAdjustmentMidPoint, intBrightnessAdjustment, intContrastAdjustment);
            intAdjustmentMidPoint->setValue(127);
            intBrightnessAdjustment->setValue(0);
            intContrastAdjustment->setValue(0);
        }
        btnMaskMode->setChecked(true);

        btnMaskMode->setToolTip("");
        btnColorMode->setToolTip("");
        btnLightnessMode->setToolTip("");
        intAdjustmentMidPoint->setToolTip("");
        intBrightnessAdjustment->setToolTip("");
        intContrastAdjustment->setToolTip("");

        if (m_hslBrushTipEnabled) {
            grpBrushMode->setToolTip("The selected brush tip does not have color channels. The brush will work in \"Mask\" mode.");
        } else {
            grpBrushMode->setToolTip("The selected brush engine does not support \"Color\" or \"Lightness\" modes. The brush will work in \"Mask\" mode.");
        }
    }


    grpBrushMode->setEnabled(modeSwitchEnabled);
    slotUpdateBrushAdjustmentsState();
    slotUpdateResetBrushAdjustmentsButtonState();
}

void KisPredefinedBrushChooser::slotUpdateBrushAdjustmentsState()
{
    const bool adjustmentsEnabled = btnLightnessMode->isEnabled() && btnLightnessMode->isChecked();

    intAdjustmentMidPoint->setEnabled(adjustmentsEnabled);
    intBrightnessAdjustment->setEnabled(adjustmentsEnabled);
    intContrastAdjustment->setEnabled(adjustmentsEnabled);
}

void KisPredefinedBrushChooser::slotUpdateResetBrushAdjustmentsButtonState()
{
    const bool adjustmentsEnabled = btnLightnessMode->isEnabled() && btnLightnessMode->isChecked();

    const bool adjustmentsDefault =
            intAdjustmentMidPoint->value() == 127 &&
            intBrightnessAdjustment->value() == 0 &&
            intContrastAdjustment->value() == 0;

    btnResetAdjustments->setEnabled(!adjustmentsDefault && adjustmentsEnabled);
}

void KisPredefinedBrushChooser::slotWriteBrushMode()
{
    KisColorfulBrush *colorfulBrush = dynamic_cast<KisColorfulBrush*>(m_brush.data());
    if (!colorfulBrush) return;

    if (btnLightnessMode->isChecked()) {
        colorfulBrush->setUseColorAsMask(true);
        colorfulBrush->setPreserveLightness(true);
    } else if (btnMaskMode->isChecked()) {
        colorfulBrush->setUseColorAsMask(true);
        colorfulBrush->setPreserveLightness(false);
    } else {
        colorfulBrush->setUseColorAsMask(false);
        colorfulBrush->setPreserveLightness(false);
    }

    emit sigBrushChanged();
}

void KisPredefinedBrushChooser::slotWriteBrushAdjustments()
{
    KisColorfulBrush *colorfulBrush = dynamic_cast<KisColorfulBrush*>(m_brush.data());
    if (!colorfulBrush) return;

    {
        // sliders emit update signals when modified from the code
        KisSignalsBlocker b(intAdjustmentMidPoint, intBrightnessAdjustment, intContrastAdjustment);
        colorfulBrush->setAdjustmentMidPoint(quint8(intAdjustmentMidPoint->value()));
        colorfulBrush->setBrightnessAdjustment(intBrightnessAdjustment->value() / 100.0);
        colorfulBrush->setContrastAdjustment(intContrastAdjustment->value() / 100.0);
    }

    emit sigBrushChanged();
}

void KisPredefinedBrushChooser::slotResetAdjustments()
{
    intAdjustmentMidPoint->setValue(127);
    intBrightnessAdjustment->setValue(0);
    intContrastAdjustment->setValue(0);

    slotWriteBrushAdjustments();
}

void KisPredefinedBrushChooser::slotNewPredefinedBrush(KoResourceSP resource)
{
    m_itemChooser->setCurrentResource(resource);
    updateBrushTip(resource);
}

void KisPredefinedBrushChooser::setBrushSize(qreal xPixels, qreal yPixels)
{
    Q_UNUSED(yPixels);
    qreal oldWidth = m_brush->width() * m_brush->scale();
    qreal newWidth = oldWidth + xPixels;

    newWidth = qMax(newWidth, qreal(0.1));

    brushSizeSpinBox->setValue(newWidth);
}

void KisPredefinedBrushChooser::setImage(KisImageWSP image)
{
    m_image = image;
}

void KisPredefinedBrushChooser::setHSLBrusTipEnabled(bool value)
{
    m_hslBrushTipEnabled = value;
}

bool KisPredefinedBrushChooser::hslBrushTipEnabled() const
{
    return m_hslBrushTipEnabled;
}

void KisPredefinedBrushChooser::slotImportNewBrushResource() {
    m_itemChooser->slotButtonClicked(KisResourceItemChooser::Button_Import);
}

void KisPredefinedBrushChooser::slotDeleteBrushResource() {
    m_itemChooser->slotButtonClicked(KisResourceItemChooser::Button_Remove);
}



#include "moc_kis_brush_chooser.cpp"



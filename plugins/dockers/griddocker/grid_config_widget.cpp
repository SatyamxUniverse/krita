/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "grid_config_widget.h"
#include "ui_grid_config_widget.h"

#include "kis_grid_config.h"
#include "kis_guides_config.h"
#include "kis_debug.h"
#include "kis_aspect_ratio_locker.h"
#include "kis_int_parse_spin_box.h"

#include <kis_icon_utils.h>
#include <kis_config.h>
#include <kis_config_notifier.h>

#include <QStandardItem>
#include <QStandardItemModel>

struct GridConfigWidget::Private
{
    Private() : guiSignalsBlocked(false) {}

    KisGridConfig gridConfig;
    KisGuidesConfig guidesConfig;
    bool guiSignalsBlocked {false};
};

GridConfigWidget::GridConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GridConfigWidget),
    m_d(new Private)
{
    ui->setupUi(this);

    ui->colorMain->setAlphaChannelEnabled(true);
    ui->colorSubdivision->setAlphaChannelEnabled(true);
    ui->colorVertical->setAlphaChannelEnabled(true);
    ui->colorGuides->setAlphaChannelEnabled(true);


    ui->angleLeftAngleSelector->setRange(0, 89);
    ui->angleRightAngleSelector->setRange(0, 89);
    ui->angleLeftAngleSelector->setDecimals(4);
    ui->angleRightAngleSelector->setDecimals(4);
    ui->angleLeftAngleSelector->setFlipOptionsMode(KisAngleSelector::FlipOptionsMode_NoFlipOptions);
    ui->angleRightAngleSelector->setFlipOptionsMode(KisAngleSelector::FlipOptionsMode_NoFlipOptions);
    ui->cellSpacingSpinbox->setSuffix(i18n(" px"));
    ui->cellSizeSpinbox->setSuffix(i18n(" px"));
    ui->loadGuidesButton->setIcon(KisIconUtils::loadIcon("folder"));
    ui->saveGuidesButton->setIcon(KisIconUtils::loadIcon("document-save-16"));
    ui->deleteAllGuidesButton->setIcon(KisIconUtils::loadIcon("edit-delete"));

    ui->chkLeftAngleActive->setChecked(true);
    ui->chkRightAngleActive->setChecked(true);
    ui->chkXSpacingActive->setChecked(true);
    ui->chkYSpacingActive->setChecked(true);

    connect(ui->deleteAllGuidesButton, SIGNAL(clicked()), SLOT(removeAllGuides()));
    connect(ui->saveGuidesButton, SIGNAL(clicked()), SLOT(saveGuides()));
    connect(ui->loadGuidesButton, SIGNAL(clicked()), SLOT(loadGuides()));

    ui->gridTypeCombobox->addItem(i18n("Rectangle"));
    ui->gridTypeCombobox->addItem(i18n("Isometric (Legacy)"));
    ui->gridTypeCombobox->addItem(i18n("Isometric"));

    ui->gridTypeCombobox->setCurrentIndex(0); // set to rectangle by default
    slotGridTypeChanged(); // update the UI to hide any elements we don't need


    connect(ui->gridTypeCombobox, SIGNAL(currentIndexChanged(int)), SLOT(slotGridTypeChanged()));

    setGridConfig(m_d->gridConfig);
    setGuidesConfig(m_d->guidesConfig);

    // hide offset UI elements if offset is disabled
    connect(ui->chkOffset, SIGNAL(toggled(bool)), ui->lblXOffset, SLOT(setVisible(bool)));
    connect(ui->chkOffset, SIGNAL(toggled(bool)), ui->lblYOffset, SLOT(setVisible(bool)));
    connect(ui->chkOffset, SIGNAL(toggled(bool)), ui->intXOffset, SLOT(setVisible(bool)));
    connect(ui->chkOffset, SIGNAL(toggled(bool)), ui->intYOffset, SLOT(setVisible(bool)));
    connect(ui->chkOffset, SIGNAL(toggled(bool)), ui->offsetAspectButton, SLOT(setVisible(bool)));


    ui->lblXOffset->setVisible(false);
    ui->lblYOffset->setVisible(false);
    ui->intXOffset->setVisible(false);
    ui->intYOffset->setVisible(false);
    ui->offsetAspectButton->setVisible(false);

    connect(ui->chkShowGrid, SIGNAL(stateChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->chkSnapToGrid, SIGNAL(stateChanged(int)), SLOT(slotGridGuiChanged()));

    connect(ui->chkShowGuides, SIGNAL(stateChanged(int)), SLOT(slotGuidesGuiChanged()));
    connect(ui->chkSnapToGuides, SIGNAL(stateChanged(int)), SLOT(slotGuidesGuiChanged()));
    connect(ui->chkLockGuides, SIGNAL(stateChanged(int)), SLOT(slotGuidesGuiChanged()));

    connect(ui->chkLeftAngleActive, SIGNAL(stateChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->chkRightAngleActive, SIGNAL(stateChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->chkXSpacingActive, SIGNAL(stateChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->chkYSpacingActive, SIGNAL(stateChanged(int)), SLOT(slotGridGuiChanged()));

    connect(ui->intSubdivision, SIGNAL(valueChanged(int)), SLOT(slotGridGuiChanged()));

    connect(ui->cellSpacingSpinbox, SIGNAL(valueChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->cellSizeSpinbox, SIGNAL(valueChanged(int)), SLOT(slotGridGuiChanged()));

    connect(ui->selectMainStyle, SIGNAL(currentIndexChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->colorMain, SIGNAL(changed(QColor)), SLOT(slotGridGuiChanged()));
    connect(ui->selectSubdivisionStyle, SIGNAL(currentIndexChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->colorSubdivision, SIGNAL(changed(QColor)), SLOT(slotGridGuiChanged()));
    connect(ui->selectVerticalStyle, SIGNAL(currentIndexChanged(int)), SLOT(slotGridGuiChanged()));
    connect(ui->colorVertical, SIGNAL(changed(QColor)), SLOT(slotGridGuiChanged()));
    connect(ui->selectGuidesStyle, SIGNAL(currentIndexChanged(int)), SLOT(slotGuidesGuiChanged()));
    connect(ui->colorGuides, SIGNAL(changed(QColor)), SLOT(slotGuidesGuiChanged()));

    ui->chkOffset->setChecked(false);

    connect(ui->chkOffset, SIGNAL(stateChanged(int)), SLOT(slotGridGuiChanged()));

    KisAspectRatioLocker *offsetLocker = new KisAspectRatioLocker(this);
    offsetLocker->connectSpinBoxes(ui->intXOffset, ui->intYOffset, ui->offsetAspectButton);

    KisAspectRatioLocker *spacingLocker = new KisAspectRatioLocker(this);
    spacingLocker->connectSpinBoxes(ui->intHSpacing, ui->intVSpacing, ui->spacingAspectButton);

    KisAspectRatioLocker *angleLocker = new KisAspectRatioLocker(this);
    angleLocker->connectAngleBoxes(ui->angleLeftAngleSelector, ui->angleRightAngleSelector, ui->angleAspectButton);

    connect(offsetLocker, SIGNAL(sliderValueChanged()), SLOT(slotGridGuiChanged()));
    connect(offsetLocker, SIGNAL(aspectButtonChanged()), SLOT(slotGridGuiChanged()));

    connect(spacingLocker, SIGNAL(sliderValueChanged()), SLOT(slotGridGuiChanged()));
    connect(spacingLocker, SIGNAL(aspectButtonChanged()), SLOT(slotGridGuiChanged()));

    connect(angleLocker, SIGNAL(sliderValueChanged()), SLOT(slotGridGuiChanged()));
    connect(angleLocker, SIGNAL(aspectButtonChanged()), SLOT(slotGridGuiChanged()));

    connect(ui->chkShowRulers,SIGNAL(toggled(bool)),SIGNAL(showRulersChanged(bool)));

    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotPreferencesUpdated()));
}

GridConfigWidget::~GridConfigWidget()
{
    delete ui;
}

void GridConfigWidget::setGridConfig(const KisGridConfig &value)
{
    KisGridConfig currentConfig = fetchGuiGridConfig();
    if (currentConfig == value) return;

    setGridConfigImpl(value);
}

void GridConfigWidget::setGuidesConfig(const KisGuidesConfig &value)
{
    KisGuidesConfig currentConfig = fetchGuiGuidesConfig();
    if (currentConfig == value) return;

    setGuidesConfigImpl(value);
}

void GridConfigWidget::setGridConfigImpl(const KisGridConfig &value)
{
    m_d->gridConfig = value;
    m_d->guiSignalsBlocked = true;

    ui->chkShowGrid->setChecked(m_d->gridConfig.showGrid());

    ui->spacingAspectButton->setKeepAspectRatio(false);
    ui->intHSpacing->setMaximum(std::numeric_limits<int>::max());
    ui->intVSpacing->setMaximum(std::numeric_limits<int>::max());
    ui->intHSpacing->setValue(m_d->gridConfig.spacing().x());
    ui->intVSpacing->setValue(m_d->gridConfig.spacing().y());
    ui->intHSpacing->setEnabled(m_d->gridConfig.xSpacingActive());
    ui->intVSpacing->setEnabled(m_d->gridConfig.ySpacingActive());

    ui->chkXSpacingActive->setChecked(m_d->gridConfig.xSpacingActive());
    ui->chkYSpacingActive->setChecked(m_d->gridConfig.ySpacingActive());

    // ensure both can't be unchecked at the same time
    ui->chkXSpacingActive->setEnabled(m_d->gridConfig.ySpacingActive());
    ui->chkYSpacingActive->setEnabled(m_d->gridConfig.xSpacingActive());

    ui->spacingAspectButton->setKeepAspectRatio(m_d->gridConfig.spacingAspectLocked());

    ui->chkOffset->setChecked(m_d->gridConfig.offsetActive());
    ui->offsetAspectButton->setKeepAspectRatio(false);
    ui->intXOffset->setValue(m_d->gridConfig.offset().x());
    ui->intYOffset->setValue(m_d->gridConfig.offset().y());
    ui->offsetAspectButton->setKeepAspectRatio(m_d->gridConfig.offsetAspectLocked());

    ui->chkLeftAngleActive->setChecked(m_d->gridConfig.angleLeftActive());
    ui->chkRightAngleActive->setChecked(m_d->gridConfig.angleRightActive());

    // ensure both can't be unchecked at the same time
    ui->chkLeftAngleActive->setEnabled(m_d->gridConfig.angleRightActive());
    ui->chkRightAngleActive->setEnabled(m_d->gridConfig.angleLeftActive());

    ui->intSubdivision->setValue(m_d->gridConfig.subdivision());
    ui->chkSnapToGrid->setChecked(m_d->gridConfig.snapToGrid());
    ui->angleAspectButton->setKeepAspectRatio(false);
    ui->angleLeftAngleSelector->setAngle(m_d->gridConfig.angleLeft());
    ui->angleRightAngleSelector->setAngle(m_d->gridConfig.angleRight());
    ui->angleLeftAngleSelector->setEnabled(m_d->gridConfig.angleLeftActive());
    ui->angleRightAngleSelector->setEnabled(m_d->gridConfig.angleRightActive());
    ui->angleAspectButton->setKeepAspectRatio(m_d->gridConfig.angleAspectLocked());
    ui->cellSpacingSpinbox->setValue(m_d->gridConfig.cellSpacing());
    ui->cellSizeSpinbox->setValue(m_d->gridConfig.cellSize());

    ui->selectMainStyle->setCurrentIndex(int(m_d->gridConfig.lineTypeMain()));
    ui->selectSubdivisionStyle->setCurrentIndex(int(m_d->gridConfig.lineTypeSubdivision()));
    ui->selectVerticalStyle->setCurrentIndex(int(m_d->gridConfig.lineTypeVertical()));
    ui->gridTypeCombobox->setCurrentIndex(int(m_d->gridConfig.gridType()));

    ui->colorMain->setColor(m_d->gridConfig.colorMain());
    ui->colorSubdivision->setColor(m_d->gridConfig.colorSubdivision());
    ui->colorVertical->setColor(m_d->gridConfig.colorVertical());

    m_d->guiSignalsBlocked = false;

    emit gridValueChanged();
}

void GridConfigWidget::setGuidesConfigImpl(const KisGuidesConfig &value)
{
    m_d->guidesConfig = value;
    m_d->guiSignalsBlocked = true;

    ui->chkShowGuides->setChecked(m_d->guidesConfig.showGuides());
    ui->chkSnapToGuides->setChecked(m_d->guidesConfig.snapToGuides());
    ui->chkLockGuides->setChecked(m_d->guidesConfig.lockGuides());

    ui->selectGuidesStyle->setCurrentIndex(int(m_d->guidesConfig.guidesLineType()));
    ui->colorGuides->setColor(m_d->guidesConfig.guidesColor());

    m_d->guiSignalsBlocked = false;

    emit guidesValueChanged();
}

KisGridConfig GridConfigWidget::gridConfig() const
{
    return m_d->gridConfig;
}

KisGuidesConfig GridConfigWidget::guidesConfig() const
{
    return m_d->guidesConfig;
}


void GridConfigWidget::removeAllGuides() {
    m_d->guidesConfig.removeAllGuides();
    emit guidesValueChanged();
}

void GridConfigWidget::saveGuides() {
    m_d->guidesConfig.saveGuides();
}

void GridConfigWidget::loadGuides() {
    m_d->guidesConfig.loadGuides();
    emit guidesValueChanged();
}

KisGridConfig GridConfigWidget::fetchGuiGridConfig() const
{
    KisGridConfig config;

    config.setShowGrid(ui->chkShowGrid->isChecked());
    config.setSnapToGrid(ui->chkSnapToGrid->isChecked());

    QPoint pt;

    pt.rx() = ui->intHSpacing->value();
    pt.ry() = ui->intVSpacing->value();
    config.setSpacing(pt);

    config.setXSpacingActive(ui->chkXSpacingActive->isChecked());
    config.setYSpacingActive(ui->chkYSpacingActive->isChecked());

    pt.rx() = ui->intXOffset->value();
    pt.ry() = ui->intYOffset->value();
    config.setOffset(pt);

    config.setOffsetActive(ui->chkOffset->isChecked());

    config.setSubdivision(ui->intSubdivision->value());
    config.setAngleLeft(ui->angleLeftAngleSelector->angle());
    config.setAngleRight(ui->angleRightAngleSelector->angle());
    config.setAngleLeftActive(ui->chkLeftAngleActive->isChecked());
    config.setAngleRightActive(ui->chkRightAngleActive->isChecked());
    config.setCellSpacing(ui->cellSpacingSpinbox->value());
    config.setCellSize(ui->cellSizeSpinbox->value());
    config.setGridType(KisGridConfig::GridType(ui->gridTypeCombobox->currentIndex()));

    config.setOffsetAspectLocked(ui->offsetAspectButton->keepAspectRatio());
    config.setSpacingAspectLocked(ui->spacingAspectButton->keepAspectRatio());
    config.setAngleAspectLocked(ui->angleAspectButton->keepAspectRatio());

    config.setLineTypeMain(KisGridConfig::LineTypeInternal(ui->selectMainStyle->currentIndex()));
    config.setLineTypeSubdivision(KisGridConfig::LineTypeInternal(ui->selectSubdivisionStyle->currentIndex()));
    config.setLineTypeVertical(KisGridConfig::LineTypeInternal(ui->selectVerticalStyle->currentIndex()));

    config.setColorMain(ui->colorMain->color());
    config.setColorSubdivision(ui->colorSubdivision->color());
    config.setColorVertical(ui->colorVertical->color());

    return config;
}

KisGuidesConfig GridConfigWidget::fetchGuiGuidesConfig() const
{
    KisGuidesConfig config = m_d->guidesConfig;

    config.setShowGuides(ui->chkShowGuides->isChecked());
    config.setSnapToGuides(ui->chkSnapToGuides->isChecked());
    config.setLockGuides(ui->chkLockGuides->isChecked());

    config.setGuidesLineType(KisGuidesConfig::LineTypeInternal(ui->selectGuidesStyle->currentIndex()));
    config.setGuidesColor(ui->colorGuides->color());

    return config;
}

void GridConfigWidget::slotGridGuiChanged()
{
    if (m_d->guiSignalsBlocked) return;

    KisGridConfig currentConfig = fetchGuiGridConfig();
    if (currentConfig == m_d->gridConfig) return;

    setGridConfigImpl(currentConfig);
}

void GridConfigWidget::slotPreferencesUpdated()
{
    KisConfig cfg(true);
    enableIsometricLegacyGrid(cfg.useOpenGL()); // Isometric view needs OpenGL
}

void GridConfigWidget::slotGuidesGuiChanged()
{
    if (m_d->guiSignalsBlocked) return;

    KisGuidesConfig currentConfig = fetchGuiGuidesConfig();
    if (currentConfig == m_d->guidesConfig) return;

    setGuidesConfigImpl(currentConfig);
}

void GridConfigWidget::slotGridTypeChanged() {
    bool showRectangleControls = ui->gridTypeCombobox->currentIndex() == 0;
    bool showIsometricLegacyControls = (ui->gridTypeCombobox->currentIndex() == 1) && !showRectangleControls;
    bool showIsometricControls = (ui->gridTypeCombobox->currentIndex() == 2) && !showRectangleControls;

    // specific rectangle UI controls
    ui->lblXSpacing->setVisible(showRectangleControls);
    ui->lblYSpacing->setVisible(showRectangleControls);
    ui->chkXSpacingActive->setVisible(showRectangleControls);
    ui->chkYSpacingActive->setVisible(showRectangleControls);
    ui->intHSpacing->setVisible(showRectangleControls);
    ui->intVSpacing->setVisible(showRectangleControls);
    ui->spacingAspectButton->setVisible(showRectangleControls);

    ui->lblSubdivision->setVisible(showRectangleControls || showIsometricControls);
    ui->intSubdivision->setVisible(showRectangleControls || showIsometricControls);
    ui->lblSubdivisionStyle->setVisible(showRectangleControls || showIsometricControls);
    ui->selectSubdivisionStyle->setVisible(showRectangleControls || showIsometricControls);
    ui->colorSubdivision->setVisible(showRectangleControls || showIsometricControls);

    // specific isometric UI controls
    ui->angleAspectButton->setVisible(showIsometricLegacyControls || showIsometricControls);
    ui->leftAngleLabel->setVisible(showIsometricLegacyControls || showIsometricControls);
    ui->chkLeftAngleActive->setVisible(showIsometricLegacyControls || showIsometricControls);
    ui->rightAngleLabel->setVisible(showIsometricLegacyControls || showIsometricControls);
    ui->chkRightAngleActive->setVisible(showIsometricLegacyControls || showIsometricControls);
    ui->angleLeftAngleSelector->setVisible(showIsometricLegacyControls || showIsometricControls);
    ui->angleRightAngleSelector->setVisible(showIsometricLegacyControls || showIsometricControls);
    ui->cellSpacingLabel->setVisible(showIsometricLegacyControls);
    ui->cellSpacingSpinbox->setVisible(showIsometricLegacyControls);
    ui->cellSizeLabel->setVisible(showIsometricControls);
    ui->cellSizeSpinbox->setVisible(showIsometricControls);

    ui->lblVerticalStyle->setVisible(showIsometricControls);
    ui->selectVerticalStyle->setVisible(showIsometricControls);
    ui->colorVertical->setVisible(showIsometricControls);

    // disable snapping for isometric grid type for now
    // remember if we had snapping enabled if it was on the rectangle mode
    if (!showRectangleControls) {
        m_isGridEnabled = ui->chkSnapToGrid->isChecked();
        ui->chkSnapToGrid->setEnabled(false);
        ui->chkSnapToGrid->setChecked(false);
    } else {
        ui->chkSnapToGrid->setEnabled(true);
        ui->chkSnapToGrid->setChecked(m_isGridEnabled);
    }

    slotGridGuiChanged();
}

bool GridConfigWidget::showRulers() const
{
    return ui->chkShowRulers->isChecked();
}

void GridConfigWidget::enableIsometricLegacyGrid(bool value)
{
    // this is related to bug: https://bugs.kde.org/show_bug.cgi?id=392526
    // not sure if it's still relevant?
    m_isIsometricGridEnabled = value;

    // Isometric (Legacy) grids disabled if OpenGL is disabled
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(ui->gridTypeCombobox->model());
    QStandardItem *item = model->item(1); // isometric (legacy) option

    // item->setFlags(m_isIsometricGridEnabled ? item->flags() & ~Qt::ItemIsEnabled:
    //                                           item->flags() | Qt::ItemIsEnabled);


     item->setEnabled(m_isIsometricGridEnabled);

     if (m_isIsometricGridEnabled) {
        item->setText(i18n("Isometric (Legacy)"));
     } else {
        item->setText(i18n("Isometric (Legacy - requires OpenGL)"));

        // change drop down index to Rectangular in case it was previously set to isometric
        ui->gridTypeCombobox->setCurrentIndex(0);
    }
}

void GridConfigWidget::setShowRulers(bool value)
{
    ui->chkShowRulers->setChecked(value);
}


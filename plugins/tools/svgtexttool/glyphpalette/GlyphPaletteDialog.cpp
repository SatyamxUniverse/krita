/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "GlyphPaletteDialog.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QHBoxLayout>
#include <KoResourcePaths.h>
#include <KoFontGlyphModel.h>
#include <KoFontRegistry.h>
#include <KoSvgText.h>

GlyphPaletteDialog::GlyphPaletteDialog(QWidget *parent)
    : KoDialog(parent)
    , m_model(new KoFontGlyphModel)
{
    setMinimumSize(500, 300);

    m_quickWidget = new QQuickWidget(this);
    this->setMainWidget(m_quickWidget);
    m_quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_quickWidget->engine()->rootContext()->setContextProperty("mainWindow", this);
    m_quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    m_quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");

    m_quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    m_quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");

    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setSource(QUrl("qrc:/GlyphPalette.qml"));

    if (m_quickWidget->rootObject()) {
        m_quickWidget->rootObject()->setProperty("titleText", "Glyph palette");
        m_quickWidget->rootObject()->setProperty("model", QVariant::fromValue(m_model));
    }
    if (!m_quickWidget->errors().empty()) {
        qWarning() << "Errors in " << windowTitle() << ":" << m_quickWidget->errors();
    }
}

void GlyphPaletteDialog::setGlyphModelFromProperties(KoSvgTextProperties properties, QString text)
{
    if (m_lastUsedProperties == properties) {
        if (m_model && m_model->rowCount() > 0) {
            QModelIndex idx = m_model->indexForString(text);
            if (idx.isValid() && m_quickWidget->rootObject()) {
                m_quickWidget->rootObject()->setProperty("currentIndex", QVariant::fromValue(idx.row()));
                return;
            }
        }
    }
    qreal res = 72.0;
    QVector<int> lengths;
    const QFont::Style style = QFont::Style(properties.propertyOrDefault(KoSvgTextProperties::FontStyleId).toInt());
    KoSvgText::AutoValue fontSizeAdjust = properties.propertyOrDefault(KoSvgTextProperties::FontSizeAdjustId).value<KoSvgText::AutoValue>();
    if (properties.hasProperty(KoSvgTextProperties::KraTextVersionId)) {
        fontSizeAdjust.isAuto = (properties.property(KoSvgTextProperties::KraTextVersionId).toInt() < 3);
    }
    QStringList families = properties.property(KoSvgTextProperties::FontFamiliesId).toStringList();
    qreal size = properties.propertyOrDefault(KoSvgTextProperties::FontSizeId).toReal();
    int weight = properties.propertyOrDefault(KoSvgTextProperties::FontWeightId).toInt();
    const std::vector<FT_FaceSP> faces = KoFontRegistry::instance()->facesForCSSValues(
        properties.property(KoSvgTextProperties::FontFamiliesId).toStringList(),
        lengths,
        properties.fontAxisSettings(),
        text,
        static_cast<quint32>(res),
        static_cast<quint32>(res),
        size,
        fontSizeAdjust.isAuto ? 1.0 : fontSizeAdjust.customValue,
        properties.propertyOrDefault(KoSvgTextProperties::FontWeightId).toInt(),
        properties.propertyOrDefault(KoSvgTextProperties::FontStretchId).toInt(),
        style != QFont::StyleNormal);
    m_model->setFace(faces.front());
    QModelIndex idx = m_model->indexForString(text);
    if (m_quickWidget->rootObject()) {
        QString name = faces.front().data()->family_name;
        m_quickWidget->rootObject()->setProperty("titleText", QString("Glyph palette: "+name));
        m_quickWidget->rootObject()->setProperty("fontFamilies", QVariant::fromValue(families));
        m_quickWidget->rootObject()->setProperty("fontSize", QVariant::fromValue(size));
        m_quickWidget->rootObject()->setProperty("fontWeight", QVariant::fromValue(weight));
        m_quickWidget->rootObject()->setProperty("fontStyle", QVariant::fromValue(style));
        if (idx.isValid()) {
            m_quickWidget->rootObject()->setProperty("currentIndex", QVariant::fromValue(idx.row()));
        }
    }
    m_lastUsedProperties = properties;
}

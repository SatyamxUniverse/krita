/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TextPropertiesDock.h"

#include <lager/state.hpp>

#include <QQuickWidget>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QFontDatabase>
#include <QStringListModel>

#include <KisViewManager.h>
#include <kis_canvas_resource_provider.h>
#include <KisStaticInitializer.h>

#include <KLocalizedContext>

#include <KoResourcePaths.h>
#include <KoCanvasResourcesIds.h>
#include <KoSvgTextPropertyData.h>
#include <text/lager/KoSvgTextPropertiesModel.h>
#include <text/lager/CssLengthPercentageModel.h>
#include <text/lager/LineHeightModel.h>
#include <lager/state.hpp>

/// Strange place to put this, do we have a better spot?
KIS_DECLARE_STATIC_INITIALIZER {
    qmlRegisterType<KoSvgTextPropertiesModel>("org.krita.flake.text", 1, 0, "KoSvgTextPropertiesModel");
    qmlRegisterType<CssLengthPercentageModel>("org.krita.flake.text", 1, 0, "CssLengthPercentageModel");
    qmlRegisterType<LineHeightModel>("org.krita.flake.text", 1, 0, "LineHeightModel");
}

struct TextPropertiesDock::Private
{
    KoSvgTextPropertiesModel *textData {new KoSvgTextPropertiesModel()};
};

TextPropertiesDock::TextPropertiesDock()
    : QDockWidget(i18n("Text Properties"))
    , d(new Private())
{
    m_quickWidget = new QQuickWidget(this);
    setWidget(m_quickWidget);
    setEnabled(true);

    m_quickWidget->engine()->rootContext()->setContextProperty("mainWindow", this);
    m_quickWidget->engine()->rootContext()->setContextObject(new KLocalizedContext(this));

    m_quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    m_quickWidget->engine()->addImportPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");

    m_quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib/qml/");
    m_quickWidget->engine()->addPluginPath(KoResourcePaths::getApplicationRoot() + "/lib64/qml/");

    m_quickWidget->setPalette(this->palette());
    m_quickWidget->setMinimumHeight(100);

    QFontDatabase fontDataBase = QFontDatabase();

    m_quickWidget->rootContext()->setContextProperty("textPropertiesModel", d->textData);
    m_quickWidget->rootContext()->setContextProperty("fontSizeModel", &d->textData->fontSizeModel);
    m_quickWidget->rootContext()->setContextProperty("lineHeightModel", &d->textData->lineHeightModel);
    m_quickWidget->rootContext()->setContextProperty("fontFamiliesModel", QVariant::fromValue(fontDataBase.families()));
    connect(d->textData, SIGNAL(textPropertyChanged()),
            this, SLOT(slotTextPropertiesChanged()));
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setSource(QUrl("qrc:/TextProperties.qml"));

}

TextPropertiesDock::~TextPropertiesDock()
{
    // Prevent double free
    m_quickWidget->setParent(nullptr);
    delete m_quickWidget;
}

void TextPropertiesDock::setViewManager(KisViewManager *kisview)
{
    KisCanvasResourceProvider* resourceProvider = kisview->canvasResourceProvider();
    connect(resourceProvider->resourceManager(), SIGNAL(canvasResourceChanged(int, QVariant)),
            this, SLOT(slotCanvasResourcesChanged(int, QVariant)));
}

void TextPropertiesDock::setCanvas(KoCanvasBase *canvas)
{
    setEnabled(true);

    if (m_canvas == canvas) {
        return;
    }

    if (m_canvas) {
        m_canvas->disconnectCanvasObserver(this);
    }

    if (!canvas) {
        m_canvas = 0;
        return;
    }

    m_canvas = dynamic_cast<KisCanvas2*>(canvas);
}

void TextPropertiesDock::unsetCanvas()
{
    setEnabled(false);
    m_canvas = 0;
}

void TextPropertiesDock::slotCanvasResourcesChanged(int key, const QVariant &value)
{
    if (key == KoCanvasResource::SvgTextPropertyData) {
        KoSvgTextPropertyData data = value.value<KoSvgTextPropertyData>();
        d->textData->textData.set(data);
    }
}

void TextPropertiesDock::slotTextPropertiesChanged()
{
    qDebug() << Q_FUNC_INFO << d->textData->textData.get();
}

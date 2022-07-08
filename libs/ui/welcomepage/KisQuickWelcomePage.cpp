#include "KisQuickWelcomePage.h"

#include <KLocalizedContext>
#include <KisMainWindow.h>
#include <KisMultiFeedRSSModel.h>
#include <KisPart.h>
#include <KisQuickImageProvider.h>
#include <KisRecentDocumentsModelWrapper.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QUrl>

class KisRecentDocumentsModelItem;

KisQuickWelcomePage::KisQuickWelcomePage(QWidget *parent)
    : QQuickWidget(parent)
{
    QQmlContext *rootCtx = engine()->rootContext();
    // Get the localization API in QML
    rootCtx->setContextObject(new KLocalizedContext(engine()));

    rootCtx->setContextProperty("welcomePage", this);

    m_tutorialsModel = new KisAtomFeedModel(this);
    // Krita's youtube feed
    m_tutorialsModel->addFeed("https://www.youtube.com/feeds/videos.xml?channel_id=UCkIccKaHDGA8lYVmUerLhag");

    // models
    rootCtx->setContextProperty("projectsModel", &KisRecentDocumentsModelWrapper::instance()->model());
    rootCtx->setContextProperty("tutorialsModel", m_tutorialsModel);

    qDebug() << engine()->importPathList();
    engine()->addImageProvider(KisQuickImageProvider::id(), new KisQuickImageProvider);

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/welcomepage/main.qml"));
}

void KisQuickWelcomePage::openProjectsUrl(QUrl url)
{
    KisMainWindow *mainWindow = KisPart::instance()->currentMainwindow();
    if (mainWindow) {
        mainWindow->openDocument(url.toLocalFile(), KisMainWindow::None);
    }
}

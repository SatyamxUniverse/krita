#include "KisQuickImageProvider.h"

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

#include <KisFileIconCreator.h>
#include <kis_debug.h>

KisQuickImageProvider::KisQuickImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage KisQuickImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    KisFileIconCreator iconCreator;

    QSize imageSize = requestedSize;
    if (requestedSize == QSize(-1, -1)) {
        // we should get the original image size
        imageSize = {INT_MAX, INT_MAX};
    }
    QImage image = iconCreator.createFilePreview(id, QGuiApplication::primaryScreen()->devicePixelRatio(),
                                                 imageSize);
    *size = image.size();

    // FIXME(sh_zam): Temporarily setting it to this.
    if (image.isNull()) {
        qWarning() << "Bad image url:" << id;
    }
    return image;
}

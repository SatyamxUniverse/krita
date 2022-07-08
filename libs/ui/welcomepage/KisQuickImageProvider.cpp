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
    // TODO(sh_zam): Should we return the requestedSize?
    Q_UNUSED(requestedSize);

    KisFileIconCreator iconCreator;

    // we set it to INT_MAX, so the size doesn't change from the original
    QImage image = iconCreator.createFilePreview(id, QGuiApplication::primaryScreen()->devicePixelRatio(),
                                                 QSize(INT_MAX, INT_MAX));
    *size = image.size();

    // FIXME(sh_zam): Temporarily setting it to this.
    if (image.isNull()) {
        qWarning() << "Bad image url:" << id;
    }
    return image;
}

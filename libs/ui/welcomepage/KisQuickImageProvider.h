#ifndef __KISQUICKIMAGEPROVIDER_H_
#define __KISQUICKIMAGEPROVIDER_H_

#include <QQuickImageProvider>

class KisQuickImageProvider : public QQuickImageProvider {
public:
    KisQuickImageProvider();

    static QString id() {
        return "kisquickimage";
    }

    static QString toProviderUrl(QString path) {
        return QString("image://") + id() + "/" + path;
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:

};

#endif // __KISQUICKIMAGEPROVIDER_H_

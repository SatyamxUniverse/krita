#ifndef __KISQUICKWELCOMEPAGE_H_
#define __KISQUICKWELCOMEPAGE_H_

#include <QUrl>
#include <QtQuickWidgets/QQuickWidget>

class KisQuickWelcomePage : public QQuickWidget
{
    Q_OBJECT

public:
    KisQuickWelcomePage(QWidget *parent);

    Q_INVOKABLE void openProjectsUrl(QUrl url);

private:
    class KisAtomFeedModel *m_tutorialsModel {0};
};

#endif // __KISQUICKWELCOMEPAGE_H_

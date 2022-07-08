#ifndef __KISQUICKWELCOMEPAGE_H_
#define __KISQUICKWELCOMEPAGE_H_

#include <QtQuickWidgets/QQuickWidget>

class KisQuickWelcomePage : public QQuickWidget
{
    Q_OBJECT

public:
    KisQuickWelcomePage(QWidget *parent);

    Q_INVOKABLE void openProjectsUrl(int path);

private:
    class KisAtomFeedModel *m_tutorialsModel {0};
};

#endif // __KISQUICKWELCOMEPAGE_H_

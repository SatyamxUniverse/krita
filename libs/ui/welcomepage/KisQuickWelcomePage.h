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

    Q_INVOKABLE void newFile();
    Q_INVOKABLE void openFile();

private:
    class KisAbstractSyndicationModel *m_tutorialsModel {0};
};

#endif // __KISQUICKWELCOMEPAGE_H_

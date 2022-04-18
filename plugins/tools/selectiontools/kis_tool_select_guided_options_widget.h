#ifndef __KIS_TOOL_SELECT_GUIDED_OPTIONS_WIDGET_H
#define __KIS_TOOL_SELECT_GUIDED_OPTIONS_WIDGET_H

#include <QScopedPointer>
#include <QWidget>
#include <QModelIndex>

#include "kis_types.h"

class KisCanvasResourceProvider;
class KoColor;

class KisToolSelectGuidedOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    KisToolSelectGuidedOptionsWidget(KisCanvasResourceProvider *provider, QWidget *parent);
    ~KisToolSelectGuidedOptionsWidget() override;

    int getKernelRadius(void);
    qreal getEpsilon(void);
    bool getSelectActive(void);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_TOOL_SELECT_GUIDED_OPTIONS_WIDGET_H */

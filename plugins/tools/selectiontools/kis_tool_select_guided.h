#ifndef KIS_TOOL_SELECT_GUIDED_H_
#define KIS_TOOL_SELECT_GUIDED_H_

#include <QPoint>
#include <QPainterPath>
#include <QScopedPointer>
#include <QWidget>
#include "kis_tool_select_guided_options_widget.h"

#include "kis_tool_paint.h"
#include "KisSelectionToolFactoryBase.h"
#include "KisToolPaintFactoryBase.h"

#include <kis_signal_compressor.h>
#include <flake/kis_node_shape.h>
#include <kis_icon.h>
#include <QKeySequence>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <KoIcon.h>

#include <kis_tool_select_base.h>
#include "selection_tools.h"
#include "kis_selection_tool_config_widget_helper.h"

#include <QMenu>

#include <Eigen/Core>
#include "filter/kis_filter.h"

class KActionCollection;
class QPainterPath;
class KoCanvasBase;
class KisSpacingInfomation;
class KisViewManager;

class KisToolSelectGuided : public KisToolSelect
{
    Q_OBJECT
public:
    KisToolSelectGuided(KoCanvasBase * canvas);
    ~KisToolSelectGuided();

    void paint(QPainter& gc, const KoViewConverter &converter) override;
    void beginPrimaryAction(KoPointerEvent *event);
    void GuidedSelection(KisPaintDeviceSP dev, KisPixelSelectionSP currentSelection, KisPixelSelectionSP newSelection, int radius, double eps);

    KisFilterConfigurationSP defaultConfiguration(KisResourcesInterfaceSP resourcesInterface) const ;

    QWidget * createOptionWidget() override;


public Q_SLOTS:

protected:

private:
    struct Private;
    const QScopedPointer<Private> d;
    int m_filterRadius;
    qreal m_epsilon;
    bool activate_selection_painting;
};

class KisToolSelectGuidedFactory : public KisSelectionToolFactoryBase
{
public:
    KisToolSelectGuidedFactory()
        : KisSelectionToolFactoryBase("KisToolSelectGuided")
    {
        setToolTip(i18n("Guided Selection Tool"));
        setSection(TOOL_TYPE_SELECTION);
        setIconName(koIconNameCStr("tool_guided_selection"));
        setPriority(9);
        setActivationShapeId(KRITA_TOOL_ACTIVATION_ID);
    }

    ~KisToolSelectGuidedFactory() override { }

    KoToolBase * createTool(KoCanvasBase *canvas) override
    {
        return new KisToolSelectGuided(canvas);
    }
};


#endif // KIS_TOOL_SELECT_GUIDED_H

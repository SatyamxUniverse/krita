#include "kis_tool_select_guided_options_widget.h"

#include "ui_kis_tool_select_guided_options_widget.h"

#include <KoColorSpaceRegistry.h>
#include "KisPaletteModel.h"

#include "kis_config.h"
#include <resources/KoColorSet.h>
#include "kis_canvas_resource_provider.h"


struct KisToolSelectGuidedOptionsWidget::Private {
    Private()
    {
    }

    Ui_KisToolSelectGuidedOptionsWidget *ui;

    int getKernelRadius(void)
    {
        return ui->kernel_radius->value();
    }
    qreal getEpsilon(void)
    {
        return ui->epsilon->value();
    }
    bool getSelectActive(void)
    {
        return ui->activate_selection_painting->checkState();
	}
};

KisToolSelectGuidedOptionsWidget::KisToolSelectGuidedOptionsWidget(KisCanvasResourceProvider */*provider*/, QWidget *parent)
    : QWidget(parent),
      m_d(new Private)
{
    m_d->ui = new Ui_KisToolSelectGuidedOptionsWidget();
    m_d->ui->setupUi(this);
}

KisToolSelectGuidedOptionsWidget::~KisToolSelectGuidedOptionsWidget()
{
}

int KisToolSelectGuidedOptionsWidget::getKernelRadius()
{
    return m_d->getKernelRadius();
}

qreal KisToolSelectGuidedOptionsWidget::getEpsilon()
{
    return m_d->getEpsilon();
}

bool KisToolSelectGuidedOptionsWidget::getSelectActive()
{
	return m_d->getSelectActive();
}

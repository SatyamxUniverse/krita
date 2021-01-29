#include "kis_tool_select_guided.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QLayout>
#include <QVBoxLayout>
#include <QBitArray>
#include <QtCore>
#include <QPolygon>

#include <kis_convolution_kernel.h>
#include <kis_convolution_painter.h>
#include <kis_sequential_iterator.h>

#include <klocalizedstring.h>
#include <KoColor.h>
#include <kis_canvas2.h>
#include "kis_cursor.h"
#include "kis_paintop_preset.h"

#include "kundo2magicstring.h"
#include "kundo2stack.h"
#include "commands_new/kis_transaction_based_command.h"
#include "kis_transaction.h"

#include "kis_processing_applicator.h"
#include "kis_datamanager.h"

#include "KoColorSpaceRegistry.h"

#include "kis_tool_select_guided_options_widget.h"
#include "libs/image/kis_paint_device_debug_utils.h"

#include "kis_paint_layer.h"
#include "kis_algebra_2d.h"

#include <kis_selection_options.h>

#include <kis_debug.h>
#include <klocalizedstring.h>

#include <KoPointerEvent.h>
#include <KoShapeController.h>
#include <KoPathShape.h>
#include <KoColorSpace.h>
#include <KoCompositeOp.h>
#include <KoViewConverter.h>

#include <kis_layer.h>
#include <kis_selection_options.h>
#include <kis_cursor.h>
#include <kis_image.h>
#include <kis_types.h>

#include <brushengine/kis_paintop_registry.h>
#include "canvas/kis_canvas2.h"
#include "kis_pixel_selection.h"
#include "kis_selection_tool_helper.h"
#include "kis_tool_select_guided_options_widget.h"

#include "kis_algebra_2d.h"

#include "KisHandlePainterHelper.h"

#include <KoColorSpace.h>
#include <KoChannelInfo.h>
#include <KoColor.h>

#include <kis_painter.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_group_layer.h>

#include <kis_slider_spin_box.h>

#include <krita_utils.h>

#include <KisDocument.h>

#include <KisViewManager.h>
#include <kis_fill_painter.h>

#include <QDebug>
struct KisToolSelectGuided::Private {
    KisToolSelectGuidedOptionsWidget *optionsWidget = nullptr;
};

KisToolSelectGuided::KisToolSelectGuided(KoCanvasBase *canvas)
    : KisToolSelect(canvas,
                    KisCursor::arrowCursor(),
                    i18n("Guided Selection")),
      d(new Private()),
      m_filterRadius(6),
      m_epsilon(0.1),
      activate_selection_painting(0)
{ }

KisToolSelectGuided::~KisToolSelectGuided()
{
}

/*
void KisToolSelectGuided::GuidedSelection(KisPaintDeviceSP dev, KisPixelSelectionSP currentSelection, KisPixelSelectionSP newSelection, const QRect & rect,int radius,double eps)
{

    const KoID target_color = RGBAColorModelID;
    const KoID target_depth = Float32BitsColorDepthID;
    const KoID target_gray = GrayColorModelID;
    const KoColorSpace *cs_currentSelection = newSelection->colorSpace();

    dev->convertTo(KoColorSpaceRegistry::instance()->colorSpace(target_color.id(),target_depth.id()));

    KisSequentialIterator test_iter(newSelection,rect);
    QVector<float> endSel(1);
    double v = 0;
    double vi = 0.0039;
    int b = 0;
    for(int x = 0 ; x<rect.width() ; x++){
        for(int y = 0 ; y<rect.height() ; y++){
            quint8* sel_pixel = test_iter.rawData();
            endSel[0]=v;
            cs_currentSelection->fromNormalisedChannelsValue(sel_pixel,endSel);
            v+=vi;
            test_iter.nextPixel();
            b++;
            if(b>255){
                b=0;
                v=0;
            };
        }
    }
}
*/



void KisToolSelectGuided::GuidedSelection(KisPaintDeviceSP dev, KisPixelSelectionSP currentSelection, KisPixelSelectionSP newSelection,int radius,double eps)
{
    QRect imgrect,rect;

    const int kernelWidth = radius;
    const int kernelHeight = radius;

    const KoID target_color = RGBAColorModelID;
    const KoID target_depth = Float32BitsColorDepthID;
    const KoID target_gray = GrayColorModelID;
    const KoColorSpace *Alpha32 = KoColorSpaceRegistry::instance()->alpha32f();
    const KoColorSpace *original_selection = newSelection->colorSpace();

    dev->convertTo(KoColorSpaceRegistry::instance()->colorSpace(target_color.id(),target_depth.id()));
    const KoColorSpace *devcol = dev->colorSpace();
    imgrect = dev->exactBounds();
    const QPoint tlpt = imgrect.topLeft();

    currentSelection->convertTo(Alpha32);

    KisPaintDeviceSP mean_rgb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
    KisPixelSelectionSP mean_p = KisPixelSelectionSP(new KisPixelSelection(*currentSelection.data()));

    const QBitArray rgbFlag = QBitArray(dev->colorSpace()->colorChannelCount());
    const QBitArray grayFlag = QBitArray(currentSelection->channelCount());


    QImage rgb_blur_kernel(kernelWidth,kernelHeight,QImage::Format_RGB32);
    rgb_blur_kernel.fill(0);
    Eigen::Matrix<qreal, Eigen::Dynamic, Eigen::Dynamic> rgb_boxKernel(kernelHeight, kernelWidth);
    for (int j = 0; j < kernelHeight; ++j) {
        for (int i = 0; i < kernelWidth; ++i) {
            rgb_boxKernel(j, i) = qRed(rgb_blur_kernel.pixel(i, j));
        }
    }

    QImage gray_blur_kernel(kernelWidth,kernelHeight,QImage::Format_Mono);
    gray_blur_kernel.fill(0);
    Eigen::Matrix<qreal, Eigen::Dynamic, Eigen::Dynamic> grayboxKernel(kernelHeight, kernelWidth);
    for (int j = 0; j < kernelHeight; ++j) {
        for (int i = 0; i < kernelWidth; ++i) {
            grayboxKernel(j, i) = qRed(rgb_blur_kernel.pixel(i, j));
        }
    }

    KisConvolutionKernelSP rgb_kernel = KisConvolutionKernel::fromMatrix(rgb_boxKernel,0,rgb_boxKernel.sum());
    KisConvolutionPainter boxblur_mean_rgb(mean_rgb);
    boxblur_mean_rgb.setChannelFlags(rgbFlag);
    boxblur_mean_rgb.applyMatrix(rgb_kernel,mean_rgb,tlpt,tlpt,imgrect.size(),BORDER_REPEAT);

    KisConvolutionKernelSP gray_kernel = KisConvolutionKernel::fromMatrix(grayboxKernel,0,grayboxKernel.sum());
    KisConvolutionPainter boxblur_mean_p(mean_p);
    boxblur_mean_p.setChannelFlags(grayFlag);
    boxblur_mean_p.applyMatrix(gray_kernel,mean_p,tlpt,tlpt,imgrect.size(),BORDER_REPEAT);

    KisSequentialIterator test_iter(mean_p,imgrect);
    KisSequentialIterator test_iter_2(newSelection,imgrect);
    QVector<float> endsel(1);
    QVector<float> endsel2(1);
    for (int x = 0 ; x < imgrect.width() ; x++){
        for (int y = 0 ; y < imgrect.height() ; x++){
            test_iter.nextPixel();
            test_iter_2.nextPixel();
            quint8* mean_p_pixel = test_iter.rawData();
            quint8* new_selection_pixel = test_iter_2.rawData();
            Alpha32->normalisedChannelsValue(mean_p_pixel,endsel);
            endsel2[0]=endsel[0];
            original_selection->fromNormalisedChannelsValue(new_selection_pixel,endsel2);
            Alpha32->fromNormalisedChannelsValue(mean_p_pixel,endsel);
        }
    }
}


void KisToolSelectGuided::beginPrimaryAction(KoPointerEvent *event)
{
    KisToolSelectBase::beginPrimaryAction(event);

    if (KisToolSelect::selectionDidMove()) {
        return;
    }

    KisCanvas2 * kisCanvas = dynamic_cast<KisCanvas2*>(canvas());
    KIS_SAFE_ASSERT_RECOVER(kisCanvas) {
        QApplication::restoreOverrideCursor();
        return;
    };

    QApplication::setOverrideCursor(KisCursor::waitCursor());

    KisSelectionToolHelper helper(kisCanvas, kundo2_i18n("Select by Foreground Extraction"));

    KisImageSP refImage = currentImage();
    KisPaintDeviceSP sourceDevice;
    sourceDevice = refImage->projection();

    KisPixelSelectionSP curSel = kisCanvas->viewManager()->selection()->pixelSelection();
    KisPixelSelectionSP tmpSel = KisPixelSelectionSP(new KisPixelSelection());
    \
    //QRect rect = curSel->selectedExactRect();

    if(curSel->isEmpty()){
        return;
    }

    GuidedSelection(sourceDevice,curSel,tmpSel,m_filterRadius,m_epsilon);
    tmpSel->invalidateOutlineCache();
    helper.selectPixelSelection(tmpSel,SELECTION_REPLACE);

    QApplication::restoreOverrideCursor();
}

void KisToolSelectGuided::paint(QPainter& gc, const KoViewConverter &converter)
{
}


QWidget * KisToolSelectGuided::createOptionWidget()
{
    KisCanvas2 * kiscanvas = dynamic_cast<KisCanvas2*>(canvas());

    d->optionsWidget = new KisToolSelectGuidedOptionsWidget(kiscanvas->viewManager()->canvasResourceProvider(), 0);
    d->optionsWidget->setObjectName(toolId() + "option widget");

    return d->optionsWidget;
}

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

void KisToolSelectGuided::GuidedSelection(KisPaintDeviceSP dev, KisPixelSelectionSP currentSelection, KisPixelSelectionSP newSelection,int radius,double eps)
{
        const int kernelWidth = radius;
        const int kernelHeight = radius;
        const int kernelWD = kernelWidth*kernelWidth;
        const double convolveFactor = 1 / static_cast<double>(kernelWD);

        QRect rect;
        const KoID target_color = RGBAColorModelID;
        const KoID target_depth = Float32BitsColorDepthID;
        const KoID target_gray = GrayColorModelID;
        const KoColorSpace *cs_currentSelection = currentSelection->colorSpace();
        const KoColorSpace *Alpha32 = KoColorSpaceRegistry::instance()->alpha32f();
        const KoColorSpace *RGB32F = KoColorSpaceRegistry::instance()->colorSpace(target_color.id(),target_depth.id());

        rect = dev->exactBounds();

        dev->convertTo(RGB32F);
        KisPaintDeviceSP mean_rgb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPaintDeviceSP var_rrrgrb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPaintDeviceSP var_gggbbb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPaintDeviceSP inv_rrrgrb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPaintDeviceSP inv_gggbbb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPaintDeviceSP mean_p_rgb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPaintDeviceSP cov_p_rgb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPaintDeviceSP a_rgb = KisPaintDeviceSP(new KisPaintDevice(*dev.data()));
        KisPixelSelectionSP mean_p = KisPixelSelectionSP(new KisPixelSelection(currentSelection.data()));
        mean_p->convertTo(Alpha32);
        KisPixelSelectionSP tmpSel = KisPixelSelectionSP(new KisPixelSelection(currentSelection.data()));

        const QBitArray grayFlag = QBitArray(newSelection->channelCount(), true);
        const QBitArray RGBFlag = QBitArray(dev->colorSpace()->colorChannelCount(),true);
        const QBitArray RGBAFlag = QBitArray(dev->colorSpace()->channelCount(),true);

        QRect boundSel= currentSelection->selectedExactRect();
        boundSel = kisGrowRect(boundSel, radius*3);
        QPoint sel_tlpt = boundSel.topLeft();

        KisSequentialIterator iter_var_rrrgrb(var_rrrgrb,boundSel);
        KisSequentialIterator iter_var_gggbbb(var_rrrgrb,boundSel);
        QVector<float> col_rrrgrb(4);
        QVector<float> col_gggbbb(4);
        QVector<float> new_col_rrrgrb(4);
        QVector<float> new_col_gggbbb(4);
        new_col_rrrgrb[3]=1;
        new_col_gggbbb[3]=1;

        for ( int x = 0 ; x < boundSel.width() ; x++){
            for ( int y = 0 ; y < boundSel.height() ; y++){
                quint8* rrrgrb_pixel = iter_var_rrrgrb.rawData();
                quint8* gggbbb_pixel = iter_var_gggbbb.rawData();
                RGB32F->normalisedChannelsValue(rrrgrb_pixel,col_rrrgrb);
                RGB32F->normalisedChannelsValue(gggbbb_pixel,col_gggbbb);
                new_col_rrrgrb[0] = col_rrrgrb[0] * col_rrrgrb[0];
                new_col_rrrgrb[1] = col_rrrgrb[0] * col_rrrgrb[1];
                new_col_rrrgrb[2] = col_rrrgrb[0] * col_rrrgrb[2];
                new_col_gggbbb[0] = col_gggbbb[1] * col_gggbbb[1];
                new_col_gggbbb[1] = col_gggbbb[1] * col_gggbbb[2];
                new_col_gggbbb[2] = col_gggbbb[2] * col_gggbbb[2];
                RGB32F->fromNormalisedChannelsValue(rrrgrb_pixel,new_col_rrrgrb);
                RGB32F->fromNormalisedChannelsValue(gggbbb_pixel,new_col_gggbbb);
                iter_var_rrrgrb.nextPixel();
                iter_var_gggbbb.nextPixel();
            }
        }


        Eigen::Matrix<qreal, Eigen::Dynamic, Eigen::Dynamic> grayboxKernel(kernelHeight, kernelWidth);
        for (int j = 0; j < kernelHeight; ++j) {
            for (int i = 0; i < kernelWidth; ++i) {
                grayboxKernel(i, j) = convolveFactor;
            }
        }

        KisConvolutionKernelSP main_box_kernel = KisConvolutionKernel::fromMatrix(grayboxKernel,0,grayboxKernel.sum());
        KisConvolutionPainter boxblur_mean_rgb(mean_rgb);
        boxblur_mean_rgb.setChannelFlags(RGBFlag);
        boxblur_mean_rgb.applyMatrix(main_box_kernel,dev,sel_tlpt,sel_tlpt,boundSel.size(),BORDER_REPEAT);

        KisSequentialIterator new_iter_var_rrrgrb(var_rrrgrb,boundSel);
        KisSequentialIterator new_iter_var_gggbbb(var_gggbbb,boundSel);
        KisSequentialIterator iter_mean_rgb(mean_rgb,boundSel);
        QVector<float> col_mean_rgb(4);
        for ( int x = 0 ; x < boundSel.width() ; x++){
            for ( int y = 0 ; y < boundSel.height() ; y++){
                quint8* var_rrrgrb_pixel = new_iter_var_rrrgrb.rawData();
                quint8* var_gggbbb_pixel = new_iter_var_gggbbb.rawData();
                quint8* mean_rgb_pixel = iter_mean_rgb.rawData();
                RGB32F->normalisedChannelsValue(var_rrrgrb_pixel,col_rrrgrb);
                RGB32F->normalisedChannelsValue(var_gggbbb_pixel,col_gggbbb);
                RGB32F->normalisedChannelsValue(mean_rgb_pixel,col_mean_rgb);
                new_col_rrrgrb[0] = col_rrrgrb[0] - col_mean_rgb[0] * col_mean_rgb[0] + eps;
                new_col_rrrgrb[1] = col_rrrgrb[1] - col_mean_rgb[0] * col_mean_rgb[1];
                new_col_rrrgrb[2] = col_rrrgrb[2] - col_mean_rgb[0] * col_mean_rgb[2];
                new_col_gggbbb[0] = col_gggbbb[0] - col_mean_rgb[1] * col_mean_rgb[1] + eps;
                new_col_gggbbb[1] = col_gggbbb[1] - col_mean_rgb[1] * col_mean_rgb[2];
                new_col_gggbbb[2] = col_gggbbb[2] - col_mean_rgb[2] * col_mean_rgb[2] + eps;
                RGB32F->fromNormalisedChannelsValue(var_rrrgrb_pixel,new_col_rrrgrb);
                RGB32F->fromNormalisedChannelsValue(var_gggbbb_pixel,new_col_gggbbb);
                new_iter_var_rrrgrb.nextPixel();
                new_iter_var_gggbbb.nextPixel();
                iter_mean_rgb.nextPixel();
            }
        }

        KisSequentialIterator val_var_rrrgrb(var_rrrgrb,boundSel);
        KisSequentialIterator val_var_gggbbb(var_gggbbb,boundSel);
        KisSequentialIterator iter_inv_rrrgrb(inv_rrrgrb,boundSel);
        KisSequentialIterator iter_inv_gggbbb(inv_gggbbb,boundSel);
        QVector<float> ref_var_rrrgrb(4);
        QVector<float> ref_var_gggbbb(4);
        QVector<float> new_inv_rrrgrb(4);
        QVector<float> new_inv_gggbbb(4);
        new_inv_rrrgrb[3]=1;
        new_inv_gggbbb[3]=1;
        float covDet;

        for ( int x = 0 ; x < boundSel.width() ; x++){
            for ( int y = 0 ; y < boundSel.height() ; y++){
                quint8* val_var_rrrgrb_pixel = val_var_rrrgrb.rawData();
                quint8* val_var_gggbbb_pixel = val_var_gggbbb.rawData();
                quint8* iter_inv_rrrgrb_pixel = iter_inv_rrrgrb.rawData();
                quint8* iter_inv_gggbbb_pixel = iter_inv_gggbbb.rawData();
                RGB32F->normalisedChannelsValue(val_var_rrrgrb_pixel,ref_var_rrrgrb);
                RGB32F->normalisedChannelsValue(val_var_gggbbb_pixel,ref_var_gggbbb);
                new_inv_rrrgrb[0]=ref_var_gggbbb[0]*ref_var_gggbbb[2]-ref_var_gggbbb[1]*ref_var_gggbbb[1];
                new_inv_rrrgrb[1]=ref_var_gggbbb[1]*ref_var_rrrgrb[2]-ref_var_rrrgrb[1]*ref_var_gggbbb[2];
                new_inv_rrrgrb[2]=ref_var_rrrgrb[1]*ref_var_gggbbb[1]-ref_var_gggbbb[2]*ref_var_rrrgrb[2];
                new_inv_gggbbb[0]=ref_var_rrrgrb[0]*ref_var_gggbbb[2]-ref_var_rrrgrb[1]*ref_var_rrrgrb[2];
                new_inv_gggbbb[1]=ref_var_rrrgrb[1]*ref_var_rrrgrb[1]-ref_var_rrrgrb[0]*ref_var_gggbbb[1];
                new_inv_gggbbb[2]=ref_var_rrrgrb[2]*ref_var_gggbbb[0]-ref_var_rrrgrb[1]*ref_var_rrrgrb[1];
                covDet=new_inv_rrrgrb[0]*ref_var_rrrgrb[0]+new_inv_rrrgrb[1]*ref_var_rrrgrb[1]+new_inv_rrrgrb[2]*ref_var_rrrgrb[2];
                if (covDet!=0){
                    new_inv_rrrgrb[0]/=covDet;
                    new_inv_rrrgrb[1]/=covDet;
                    new_inv_rrrgrb[2]/=covDet;
                    new_inv_gggbbb[0]/=covDet;
                    new_inv_gggbbb[1]/=covDet;
                    new_inv_gggbbb[2]/=covDet;
                }
                RGB32F->fromNormalisedChannelsValue(iter_inv_rrrgrb_pixel,new_inv_rrrgrb);
                RGB32F->fromNormalisedChannelsValue(iter_inv_gggbbb_pixel,new_inv_gggbbb);
                val_var_rrrgrb.nextPixel();
                val_var_gggbbb.nextPixel();
                iter_inv_rrrgrb.nextPixel();
                iter_inv_gggbbb.nextPixel();
            }
        }

        KisConvolutionPainter boxblur_mean_p(mean_p);
        boxblur_mean_p.setChannelFlags(grayFlag);
        boxblur_mean_p.applyMatrix(main_box_kernel,currentSelection,sel_tlpt,sel_tlpt,boundSel.size(),BORDER_REPEAT);

        KisSequentialIterator iter_mean_p_rgb(mean_p_rgb,boundSel);
        KisSequentialIterator val_p(mean_p,boundSel);
        QVector<float> src_mean_p_rgb(4);
        QVector<float> src_p(1);
        QVector<float> dst_mean_p_rgb(4);
        dst_mean_p_rgb[3]=1;

        for ( int x = 0 ; x < boundSel.width() ; x++){
            for ( int y = 0 ; y < boundSel.height() ; y++){
                quint8* mean_p_rgb_pixel = iter_mean_p_rgb.rawData();
                quint8* src_mean_p_pixel = val_p.rawData();
                RGB32F->normalisedChannelsValue(mean_p_rgb_pixel,src_mean_p_rgb);
                Alpha32->normalisedChannelsValue(src_mean_p_pixel,src_p);
                dst_mean_p_rgb[0]=src_mean_p_rgb[0] * src_p[0];
                dst_mean_p_rgb[1]=src_mean_p_rgb[1] * src_p[0];
                dst_mean_p_rgb[2]=src_mean_p_rgb[2] * src_p[0];
                RGB32F->fromNormalisedChannelsValue(mean_p_rgb_pixel,dst_mean_p_rgb);
                iter_mean_rgb.nextPixel();
                val_p.nextPixel();
            }
        }

        KisSequentialIterator iter_mean_p_rgb_2(mean_p_rgb,boundSel);
        KisSequentialIterator iter_mean_rgb_2(mean_rgb,boundSel);
        KisSequentialIterator iter_mean_p_2(mean_p,boundSel);
        KisSequentialIterator iter_inv_rrrgrb_2(inv_rrrgrb,boundSel);
        KisSequentialIterator iter_inv_gggbbb_2(inv_rrrgrb,boundSel);
        KisSequentialIterator iter_a_rgb(a_rgb,boundSel);
        QVector<float> src_p_rgb(4);
        QVector<float> src_mean_rgb(4);
        QVector<float> src_mean_p(1);
        QVector<float> src_inv_rrrgrb(4);
        QVector<float> src_inv_gggbbb(4);
        QVector<float> dst_cov_p_rgb(4);
        QVector<float> dst_a_rgb(4);
        dst_cov_p_rgb[3]=1;
        for ( int x = 0 ; x < boundSel.width() ; x++){
            for ( int y = 0 ; y < boundSel.height() ; y++){
                quint8* mean_p_rgb_2_pixel = iter_mean_p_rgb_2.rawData();
                quint8* mean_rgb_2_pixel = iter_mean_rgb_2.rawData();
                quint8* mean_p_2_pixel = iter_mean_p_2.rawData();
                quint8* inv_rrrgrb_pixel_2=iter_inv_rrrgrb_2.rawData();
                quint8* inv_gggbbb_pixel_2=iter_inv_gggbbb_2.rawData();
                quint8* a_rgb_pixel = iter_a_rgb.rawData();
                RGB32F->normalisedChannelsValue(mean_p_rgb_2_pixel,src_p_rgb);
                RGB32F->normalisedChannelsValue(mean_rgb_2_pixel,src_mean_rgb);
                RGB32F->normalisedChannelsValue(inv_rrrgrb_pixel_2,src_inv_rrrgrb);
                RGB32F->normalisedChannelsValue(inv_gggbbb_pixel_2,src_inv_gggbbb);
                Alpha32->normalisedChannelsValue(mean_p_2_pixel,src_mean_p);
                dst_cov_p_rgb[0]=src_p_rgb[0]-src_mean_rgb[0]*src_mean_p[0];
                dst_cov_p_rgb[1]=src_p_rgb[1]-src_mean_rgb[1]*src_mean_p[0];
                dst_cov_p_rgb[2]=src_p_rgb[2]-src_mean_rgb[2]*src_mean_p[0];
                dst_a_rgb[0]=src_inv_rrrgrb[0]*dst_cov_p_rgb[0]+src_inv_rrrgrb[1]*dst_cov_p_rgb[1]+src_inv_rrrgrb[2]*dst_cov_p_rgb[2];
                dst_a_rgb[1]=src_inv_rrrgrb[1]*dst_cov_p_rgb[0]+src_inv_gggbbb[0]*dst_cov_p_rgb[1]+src_inv_gggbbb[1]*dst_cov_p_rgb[2];
                dst_a_rgb[2]=src_inv_rrrgrb[2]*dst_cov_p_rgb[0]+src_inv_gggbbb[1]*dst_cov_p_rgb[1]+src_inv_gggbbb[2]*dst_cov_p_rgb[2];
                dst_a_rgb[3]=src_mean_p[0] - dst_a_rgb[0]*src_mean_rgb[0] - dst_a_rgb[1]*src_mean_rgb[1] - dst_a_rgb[2]*src_mean_rgb[2]; //cv::Mat b [Eqn.15]
                RGB32F->fromNormalisedChannelsValue(a_rgb_pixel,dst_a_rgb);
                iter_mean_p_rgb_2.nextPixel();
                iter_mean_rgb_2.nextPixel();
                iter_mean_p_2.nextPixel();
                iter_inv_rrrgrb_2.nextPixel();
                iter_inv_gggbbb_2.nextPixel();
                iter_a_rgb.nextPixel();
            }
        }

        KisConvolutionPainter boxblur_a_rgb(a_rgb);
        boxblur_mean_p.setChannelFlags(RGBAFlag);
        boxblur_mean_p.applyMatrix(main_box_kernel,a_rgb,sel_tlpt,sel_tlpt,boundSel.size(),BORDER_REPEAT);

        KisSequentialIterator iter_a_rgb_end(a_rgb,boundSel);
        KisSequentialIterator iter_dev(dev,boundSel);
        KisSequentialIterator iter_tmpSel(tmpSel,boundSel);
        QVector<float> src_a_rgb_end(4);
        QVector<float> src_dev(4);
        QVector<float> dst_tmpSel(1);

        for ( int x = 0 ; x < boundSel.width() ; x++){
            for ( int y = 0 ; y < boundSel.height() ; y++){
                quint8* src_a_rgb_pixel = iter_a_rgb_end.rawData();
                quint8* src_iter_dev_pixel = iter_tmpSel.rawData();
                quint8* tmp_pixel = iter_tmpSel.rawData();
                RGB32F->normalisedChannelsValue(src_a_rgb_pixel,src_a_rgb_end);
                RGB32F->normalisedChannelsValue(src_iter_dev_pixel,src_dev);
                dst_tmpSel[0]=src_a_rgb_end[0] * src_dev[0]
                        + src_a_rgb_end[1] * src_dev[1]
                        + src_a_rgb_end[2] * src_dev[2]
                        + src_a_rgb_end[3];
                Alpha32->fromNormalisedChannelsValue(tmp_pixel,dst_tmpSel);
                iter_a_rgb_end.nextPixel();
                iter_dev.nextPixel();
                iter_tmpSel.nextPixel();
            }
        }

        tmpSel->convertTo(cs_currentSelection);

        newSelection->applySelection(tmpSel, SELECTION_REPLACE);
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

    m_filterRadius = d->optionsWidget->getKernelRadius();
    m_epsilon = d->optionsWidget->getEpsilon();

    GuidedSelection(sourceDevice,curSel,tmpSel,m_filterRadius,m_epsilon);
    tmpSel->invalidateOutlineCache();
    helper.selectPixelSelection(tmpSel,SELECTION_REPLACE);

    KisPixelSelectionSP selAfter = kisCanvas->viewManager()->selection()->pixelSelection();

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

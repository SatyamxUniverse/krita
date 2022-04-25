/*
 *  Copyright (c) 2020 Scott Petrovic <scottpetrovic@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "PaintingResources.h"

#include <kis_types.h>

#include "KisView.h"
#include "kis_types.h"

#include "kis_canvas_resource_provider.h"
#include "kis_paintop_preset.h"

#include "KisViewManager.h"
#include "KisGlobalResourcesInterface.h"

#include <KoResourcePaths.h>

#include "Document.h"

#include <KisPart.h>
#include <KisMainWindow.h>

#include <kis_types.h>
#include <kis_annotation.h>



#include "kis_animation_importer.h"
#include <kis_canvas2.h>
#include <KoUpdater.h>
#include <QMessageBox>


#include "strokes/KisFreehandStrokeInfo.h"
#include "kis_resources_snapshot.h"
#include "kis_canvas_resource_provider.h"
#include "strokes/freehand_stroke.h"
#include "kis_painting_information_builder.h"
#include "KisAsyncronousStrokeUpdateHelper.h"
#include "kis_stroke_strategy.h"



void PaintingResources::addStrokeJob(KisImageWSP image, KisStrokeJobData *data)
{
    KisStrokeId strokeId = setupStrokeForPainting(image);
    image->addJob(strokeId, data);
    image->addJob(strokeId, new KisAsyncronousStrokeUpdateHelper::UpdateData(true));
    image->endStroke(strokeId);
}


KisStrokeId PaintingResources::setupStrokeForPainting(KisImageWSP image)
{
    // need to grab the resource provider
    KisView *activeView = KisPart::instance()->currentMainwindow()->activeView();
    KoCanvasResourceProvider *resourceManager = activeView->viewManager()->canvasResourceProvider()->resourceManager();

    // grab the image and current layer
    KisNodeSP node = activeView->currentNode();

    KisResourcesSnapshotSP resources = new KisResourcesSnapshot(image, node, resourceManager);
    KisFreehandStrokeInfo *strokeInfo = new KisFreehandStrokeInfo();
    KisStrokeStrategy* strokeStrategy =  new FreehandStrokeStrategy(resources, strokeInfo, kundo2_noi18n("python_stroke"));

    return image->startStroke( strokeStrategy );
}

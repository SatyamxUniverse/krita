/*
 *  SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ShapeTreeDockerDock.h"

#include "KisShapeTreeModel.h"

#include <KisViewManager.h>
#include <KoSelectedShapesProxy.h>
#include <KoSelection.h>
#include <KoShapeManager.h>
#include <kis_canvas2.h>
#include <kis_node_manager.h>

#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

#include <memory>

ShapeTreeDockerDock::ShapeTreeDockerDock()
    : QDockWidget(i18n("Vector Shapes Tree"))
    , m_treeView{new QTreeView(this)}
{
    m_treeView->setEnabled(false);
    m_treeView->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setHeaderHidden(true);
    m_treeView->setIconSize({16, 16});
    setWidget(m_treeView);
}

ShapeTreeDockerDock::~ShapeTreeDockerDock() = default;

QString ShapeTreeDockerDock::observerName()
{
    return QLatin1String("ShapeTreeDockerDock");
}

void ShapeTreeDockerDock::setCanvas(KoCanvasBase *canvas)
{
    if (m_canvas == canvas) {
        return;
    }
    if (m_canvas) {
        m_canvas->disconnectCanvasObserver(this);
        m_canvasConnections.clear();
    }

    m_canvas = qobject_cast<KisCanvas2*>(canvas);
    if (m_canvas) {
        m_canvasConnections.addConnection(m_canvas->viewManager()->nodeManager(),
                                          &KisNodeManager::sigNodeActivated,
                                          this,
                                          &ShapeTreeDockerDock::slotNodeActivated);
        const KisNodeSP node = m_canvas->viewManager()->nodeManager()->activeNode();
        slotNodeActivated(node);
    } else {
        slotNodeActivated(nullptr);
    }
}

void ShapeTreeDockerDock::unsetCanvas()
{
    setCanvas(nullptr);
}

void ShapeTreeDockerDock::slotNodeActivated(KisNodeSP node)
{
    if (m_shapeLayer.data() == node) {
        return;
    }
    if (m_shapeLayer) {
        m_shapeLayerConnections.clear();
        m_treeView->selectionModel()->disconnect(this);
    }

    // Calling setModel on a view leaks the old selection model, even the Qt
    // doc recommends manually deleting it after setting the new model, so do
    // it here with the help of unique_ptr.
    const std::unique_ptr<QItemSelectionModel> oldSelectionModel{m_treeView->selectionModel()};

    // Keep the old model valid until after the new one has been set to the view.
    const std::unique_ptr<KisShapeTreeModel> oldModel = std::move(m_shapeTreeModel);

    m_shapeLayer = qobject_cast<KisShapeLayer *>(node.data());
    if (m_shapeLayer) {
        m_shapeTreeModel = std::make_unique<KisShapeTreeModel>(m_shapeLayer);
        m_treeView->setModel(m_shapeTreeModel.get());
        m_shapeLayerConnections.addConnection(m_shapeLayer->selectedShapesProxy(),
                                              &KoSelectedShapesProxy::selectionChanged,
                                              this,
                                              &ShapeTreeDockerDock::slotLayerSelectionChanged);
        connect(m_treeView->selectionModel(),
                &QItemSelectionModel::selectionChanged,
                this,
                &ShapeTreeDockerDock::slotTreeViewSelectionChanged);
        slotLayerSelectionChanged();
        m_treeView->setEnabled(true);
    } else {
        m_treeView->setEnabled(false);
        m_treeView->setModel(nullptr);
    }
}

void ShapeTreeDockerDock::slotLayerSelectionChanged()
{
    const QList<KoShape *> selectedShapes = m_shapeLayer->selectedShapesProxy()->selection()->selectedShapes();
    QItemSelection selection;
    Q_FOREACH (KoShape *const shape, selectedShapes) {
        const QModelIndex idx = m_shapeTreeModel->indexForShape(shape);
        if (idx.isValid()) {
            selection.select(idx, idx);
        }
    }
    m_treeView->selectionModel()->select(selection, QItemSelectionModel::SelectionFlag::ClearAndSelect);
}

void ShapeTreeDockerDock::slotTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    KoSelection *shapeSelection = m_shapeLayer->selectedShapesProxy()->selection();
    Q_FOREACH (const QModelIndex &idx, deselected.indexes()) {
        shapeSelection->deselect(m_shapeTreeModel->shapeForIndex(idx));
    }
    Q_FOREACH (const QModelIndex &idx, selected.indexes()) {
        shapeSelection->select(m_shapeTreeModel->shapeForIndex(idx));
    }
}

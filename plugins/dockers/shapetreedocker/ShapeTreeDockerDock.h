/*
 *  SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <KoCanvasObserverBase.h>
#include <kis_signal_auto_connection.h>
#include <kis_shared_ptr.h>

#include <QDockWidget>
#include <QPointer>

#include <memory>

class QItemSelection;
class QTreeView;
class KisCanvas2;
class KisNode;
class KisShapeLayer;
class KisShapeTreeModel;

using KisNodeSP = KisSharedPtr<KisNode>;
using KisShapeLayerSP = KisSharedPtr<KisShapeLayer>;

class ShapeTreeDockerDock : public QDockWidget, public KoCanvasObserverBase
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ShapeTreeDockerDock)

public:
    ShapeTreeDockerDock();
    ~ShapeTreeDockerDock() override;

    /* KoCanvasObserverBase */

    QString observerName() override;
    void setCanvas(KoCanvasBase *canvas) override;
    void unsetCanvas() override;

private Q_SLOTS:
    void slotNodeActivated(KisNodeSP node);
    void slotLayerSelectionChanged();
    void slotTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    QTreeView *m_treeView;
    std::unique_ptr<KisShapeTreeModel> m_shapeTreeModel;
    QPointer<KisCanvas2> m_canvas;
    KisSignalAutoConnectionsStore m_canvasConnections;
    KisShapeLayerSP m_shapeLayer;
    KisSignalAutoConnectionsStore m_shapeLayerConnections;
};

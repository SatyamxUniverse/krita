/*
 *  SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <kis_shape_layer.h>

#include <QAbstractItemModel>

#include <vector>

class KisShapeTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(KisShapeTreeModel)

    class Node;

public:
    explicit KisShapeTreeModel(KisShapeLayerSP shapeLayer, QObject *parent = nullptr);
    ~KisShapeTreeModel() override;

private Q_SLOTS:
    void slotShapeHasBeenAddedToHierarchy(KoShape *shape, KoShapeContainer *addedToSubtree);
    void slotShapeToBeRemovedFromHierarchy(KoShape *shape, KoShapeContainer *removedFromSubtree);

private:
    [[nodiscard]] static Node *nodeForIndex(const QModelIndex &index);

public:
    [[nodiscard]] static KoShape *shapeForIndex(const QModelIndex &index);
    [[nodiscard]] QModelIndex indexForShape(KoShape *shape) const;

public:

    /* QAbstractItemModel */

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    class Node
    {
        Q_DISABLE_COPY(Node)

    public:
        explicit Node(KoShape *shape);
        ~Node();

        [[nodiscard]] KoShape *shape() const;
        [[nodiscard]] bool isInitialized() const;
        [[nodiscard]] std::vector<std::unique_ptr<Node>> &children();
        void reset();
        [[nodiscard]] Node *findNodeForShape(KoShape *shape);

    private:
        KoShape *m_shape;
        mutable std::optional<std::vector<std::unique_ptr<Node>>> m_children;
    };

private:
    KisShapeLayerSP m_shapeLayer;
    mutable Node m_rootNode;
};

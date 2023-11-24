/*
 *  SPDX-FileCopyrightText: 2023 Alvin Wong <alvin@alvinhc.com>
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisShapeTreeModel.h"

#include <KoPathShape.h>
#include <KoShapeGroup.h>
#include <KoSvgTextShape.h>
#include <kis_icon_utils.h>

#include <algorithm>
#include <typeinfo>
#include <vector>
#include <utility>


/* KisShapeTreeModel::Node */

KisShapeTreeModel::Node::Node(KoShape *shape)
    : m_shape(shape)
    , m_children{}
{}

KisShapeTreeModel::Node::~Node() = default;

KoShape *KisShapeTreeModel::Node::shape() const
{
    return m_shape;
}

bool KisShapeTreeModel::Node::isInitialized() const
{
    return m_children.has_value();
}

std::vector<std::unique_ptr<KisShapeTreeModel::Node>> &KisShapeTreeModel::Node::children()
{
    if (!m_children) {
        m_children.emplace();
        if (KoShapeContainer *const container = dynamic_cast<KoShapeContainer *>(m_shape)) {
            const QList<KoShape *> shapes = container->shapes();
            m_children->reserve(shapes.count());
            Q_FOREACH (KoShape *const child, shapes) {
                m_children->push_back(std::make_unique<Node>(child));
            }
        }
    }
    return *m_children;
}

KisShapeTreeModel::Node *KisShapeTreeModel::Node::findNodeForShape(KoShape *shape)
{
    if (m_shape == shape) {
        return this;
    }
    if (KoShape *const parent = shape->parent()) {
        if (m_shape == parent) {
            for (std::unique_ptr<Node> &child : this->children()) {
                if (child->shape() == shape) {
                    return child.get();
                }
            }
            qWarning() << __PRETTY_FUNCTION__ << "Shape not found in children";
        }
        if (Node *const parentNode = findNodeForShape(parent)) {
            return parentNode->findNodeForShape(shape);
        }
    }
    return nullptr;
}


/* KisShapeTreeModel */

KisShapeTreeModel::KisShapeTreeModel(KisShapeLayerSP shapeLayer, QObject *parent)
    : QAbstractItemModel(parent)
    , m_shapeLayer(shapeLayer)
    , m_rootNode(shapeLayer.data())
{
    connect(m_shapeLayer, &KisShapeLayer::sigShapeHasBeenAddedToHierarchy,
            this, &KisShapeTreeModel::slotShapeHasBeenAddedToHierarchy);
    connect(m_shapeLayer, &KisShapeLayer::sigShapeToBeRemovedFromHierarchy,
            this, &KisShapeTreeModel::slotShapeToBeRemovedFromHierarchy);
}

KisShapeTreeModel::~KisShapeTreeModel()
{
    m_shapeLayer->disconnect(this);
}

void KisShapeTreeModel::slotShapeHasBeenAddedToHierarchy(KoShape *shape, KoShapeContainer */*addedToSubtree*/)
{
    KoShapeContainer *const parentShape = shape->parent();
    Node *parentNode{};
    QModelIndex parentIndex;
    if (parentShape == m_shapeLayer) {
        parentNode = &m_rootNode;
    } else {
        parentIndex = indexForShape(parentShape);
        if (Q_UNLIKELY(!parentIndex.isValid())) {
            qWarning() << __PRETTY_FUNCTION__ << "Failed to get index for parent shape" << parentShape << parentShape->shapeId() << parentShape->name();
            return;
        }
        parentNode = nodeForIndex(parentIndex);
    }
    const int idx = parentShape->shapes().lastIndexOf(shape);
    if (Q_UNLIKELY(idx < 0)) {
        qWarning() << __PRETTY_FUNCTION__ << "Shape not found in its parent";
        return;
    }
    beginInsertRows(parentIndex, idx, idx);
    parentNode->children().insert(parentNode->children().cbegin() + idx, std::make_unique<Node>(shape));
    endInsertRows();
}

void KisShapeTreeModel::slotShapeToBeRemovedFromHierarchy(KoShape *shape, KoShapeContainer */*removedFromSubtree*/)
{
    KoShapeContainer *const parentShape = shape->parent();
    Node *parentNode{};
    QModelIndex parentIndex;
    if (parentShape == m_shapeLayer) {
        parentNode = &m_rootNode;
    } else {
        parentIndex = indexForShape(parentShape);
        if (Q_UNLIKELY(!parentIndex.isValid())) {
            qWarning() << __PRETTY_FUNCTION__ << "Failed to get index for parent shape" << parentShape << parentShape->shapeId() << parentShape->name();
            return;
        }
        parentNode = nodeForIndex(parentIndex);
    }
    const int idx = parentShape->shapes().indexOf(shape);
    if (Q_UNLIKELY(idx < 0)) {
        qWarning() << __PRETTY_FUNCTION__ << "Shape not found in its parent";
        return;
    }
    beginRemoveRows(parentIndex, idx, idx);
    parentNode->children().erase(parentNode->children().cbegin() + idx);
    endRemoveRows();
}

KisShapeTreeModel::Node *KisShapeTreeModel::nodeForIndex(const QModelIndex &index)
{
    if (Q_UNLIKELY(!index.isValid())) {
        qWarning() << __PRETTY_FUNCTION__ << "Invalid model index";
        return nullptr;
    }
    return reinterpret_cast<Node *>(index.internalPointer());
}

KoShape *KisShapeTreeModel::shapeForIndex(const QModelIndex &index)
{
    return nodeForIndex(index)->shape();
}

[[nodiscard]] QModelIndex KisShapeTreeModel::indexForShape(KoShape *shape) const
{
    if (shape == m_shapeLayer) {
        // Root shape cannot be referred to by a model index.
        return {};
    }
    if (KoShape *const parentShape = shape->parent()) {
        Node *parentNode{};
        if (parentShape == m_shapeLayer) {
            parentNode = &m_rootNode;
        } else {
            parentNode = m_rootNode.findNodeForShape(parentShape);
            if (!parentNode) {
                return {};
            }
        }
        Node *const shapeNode = parentNode->findNodeForShape(shape);
        auto &children = parentNode->children();
        const auto found = std::find_if(children.cbegin(), children.cend(), [&](auto &node) {
            return node.get() == shapeNode;
        });
        return createIndex(static_cast<int>(found - children.cbegin()), 0, shapeNode);
    }
    // qWarning() << "Shape not found in tree hierarchy";
    return {};
}

QModelIndex KisShapeTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column > 0) {
        return {};
    }
    Node *parentNode{};
    if (parent.isValid()) {
        parentNode = nodeForIndex(parent);
    } else {
        parentNode = &m_rootNode;
    }
    std::vector<std::unique_ptr<Node>> &children = parentNode->children();
    if (static_cast<size_t>(row) >= children.size()) {
        return {};
    }
    return createIndex(row, column, children[row].get());
}

QModelIndex KisShapeTreeModel::parent(const QModelIndex &index) const
{
    return indexForShape(shapeForIndex(index)->parent());
}

Qt::ItemFlags KisShapeTreeModel::flags(const QModelIndex &index) const
{
    KoShape *const shape = shapeForIndex(index);
    if (typeid(*shape) == typeid(KoSvgTextChunkShape)) {
        // Disallow selecting inner chunks of a text shape.
        // FIXME: Is there a better way to check for this?
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant KisShapeTreeModel::data(const QModelIndex &index, int role) const
{
    KoShape *const shape = shapeForIndex(index);
    switch (role) {
    case Qt::DisplayRole: {
        // FIXME: Remove pointer
        // FIXME: Refactor the shape type display name part into KoShape
        // FIXME: Handle empty name (#id) better
        QString str;
        QTextStream ts(&str);
        ts << shape << " ";
        if (KoPathShape *const pathShape = dynamic_cast<KoPathShape *>(shape)) {
            ts << pathShape->pathShapeId();
        } else {
            ts << shape->shapeId();
        }
        ts << " #" << shape->name();
        ts.flush();
        return str;
    }
    case Qt::SizeHintRole:
        return QSize(0, 20);
    case Qt::DecorationRole:
        // FIXME: Refactor into KoShape
        if (typeid(*shape) == typeid(KoSvgTextShape) || typeid(*shape) == typeid(KoSvgTextChunkShape)) {
            return KisIconUtils::loadIcon(QLatin1String("draw-text"));
        }
        if (typeid(*shape) == typeid(KoShapeGroup)) {
            return KisIconUtils::loadIcon(QLatin1String("tool_rect_selection"));
        }
        if (KoPathShape *const pathShape = dynamic_cast<KoPathShape *>(shape)) {
            if (pathShape->pathShapeId() == QLatin1String("RectangleShape")) {
                return KisIconUtils::loadIcon(QLatin1String("krita_tool_rectangle"));
            }
            if (pathShape->pathShapeId() == QLatin1String("EllipseShape")) {
                return KisIconUtils::loadIcon(QLatin1String("krita_tool_ellipse"));
            }
            // TODO: more shapes
            return KisIconUtils::loadIcon(QLatin1String("polyline"));
        }
        break;
    }
    return {};
}

int KisShapeTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return nodeForIndex(parent)->children().size();
    }
    return m_rootNode.children().size();
}

int KisShapeTreeModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}

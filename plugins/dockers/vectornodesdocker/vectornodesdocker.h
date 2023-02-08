#ifndef _VECTORNODES_DOCKER_H_
#define _VECTORNODES_DOCKER_H_

#include <QObject>
#include <QVariant>

class KisViewManager;

/**
 * Template of view plugin
 */
class VectorNodesDockerPlugin : public QObject {
    Q_OBJECT
    public:
        VectorNodesDockerPlugin(QObject *parent, const QVariantList &);
        ~VectorNodesDockerPlugin() override;
    private:
        KisViewManager* m_view {nullptr};
};

#endif

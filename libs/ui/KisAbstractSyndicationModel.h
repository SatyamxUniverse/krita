#ifndef __KISABSTRACTSYNDICATIONMODEL_H_
#define __KISABSTRACTSYNDICATIONMODEL_H_

#include <QAbstractListModel>

#include <KisRssReader.h>

class QNetworkReply;
class QNetworkAccessManager;
class KisNetworkAccessManager;

class KRITAUI_EXPORT KisAbstractSyndicationModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int articleCount READ articleCount WRITE setArticleCount NOTIFY articleCountChanged)
public:
    explicit KisAbstractSyndicationModel(QObject *parent = 0);
    explicit KisAbstractSyndicationModel(KisNetworkAccessManager* nam, QObject *parent = 0);
    ~KisAbstractSyndicationModel() override;

    QHash<int, QByteArray> roleNames() const override;
    virtual void addFeed(const QString& feed);
    void removeFeed(const QString& feed);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual RssItemList parse(QNetworkReply *reply) = 0;

    int articleCount() const {
        return m_articleCount;
    }

public Q_SLOTS:
    void setArticleCount(int arg) {
        if (m_articleCount != arg) {
            m_articleCount = arg;
            emit articleCountChanged(arg);
        }
    }

Q_SIGNALS:
    void articleCountChanged(int arg);
    void feedDataChanged();

private Q_SLOTS:
    void appendFeedData(QNetworkReply *reply);

protected:
    QStringList m_sites;
    RssItemList m_aggregatedFeed;
    QNetworkAccessManager *m_networkAccessManager;
    int m_articleCount;

    void sortAggregatedFeed();
    void initialize();
};

#endif // __KISABSTRACTSYNDICATIONMODEL_H_

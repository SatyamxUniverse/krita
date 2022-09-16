#include "KisAbstractSyndicationModel.h"

#include <QTimer>
#include <QThread>
#include <QXmlStreamReader>
#include <QCoreApplication>
#include <QLocale>
#include <QFile>
#include <QTextBlock>
#include <QTextDocument>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <KisNetworkAccessManager.h>

#include <KisRssReader.h>

KisAbstractSyndicationModel::KisAbstractSyndicationModel(QObject *parent) :
    QAbstractListModel(parent),
    m_networkAccessManager(new KisNetworkAccessManager),
    m_articleCount(0)
{
    initialize();
}

KisAbstractSyndicationModel::KisAbstractSyndicationModel(KisNetworkAccessManager* nam, QObject* parent)
    : QAbstractListModel(parent),
      m_networkAccessManager(nam),
      m_articleCount(0)
{
    initialize();
}


KisAbstractSyndicationModel::~KisAbstractSyndicationModel()
{
    delete m_networkAccessManager;
}

QHash<int, QByteArray> KisAbstractSyndicationModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[KisRssReader::RssRoles::TitleRole] = "title";
    roleNames[KisRssReader::RssRoles::DescriptionRole] = "description";
    roleNames[KisRssReader::RssRoles::PubDateRole] = "pubDate";
    roleNames[KisRssReader::RssRoles::LinkRole] = "url";
    roleNames[KisRssReader::RssRoles::CategoryRole] = "category";
    roleNames[KisRssReader::RssRoles::BlogNameRole] = "blogName";
    roleNames[KisRssReader::RssRoles::BlogIconRole] = "blogIcon";
    roleNames[KisRssReader::RssRoles::ThumbnailLinkRole] = "thumbnail";
    return roleNames;
}

void KisAbstractSyndicationModel::addFeed(const QString& feed)
{
    if (m_sites.contains(feed)) {
        // do not add the feed twice
        return;
    }

    m_sites << feed;
    const QUrl feedUrl(feed);
    m_networkAccessManager->getUrl(feedUrl);
}

bool sortForPubDate(const RssItem& item1, const RssItem& item2)
{
    return item1.pubDate > item2.pubDate;
}

void KisAbstractSyndicationModel::appendFeedData(QNetworkReply *reply)
{
    beginResetModel();
    m_aggregatedFeed.append(parse(reply));
    sortAggregatedFeed();
    setArticleCount(m_aggregatedFeed.size());
    endResetModel();

    emit feedDataChanged();
}

void KisAbstractSyndicationModel::sortAggregatedFeed()
{
    std::sort(m_aggregatedFeed.begin(), m_aggregatedFeed.end(), sortForPubDate);
}

void KisAbstractSyndicationModel::initialize()
{
    connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)),
            SLOT(appendFeedData(QNetworkReply*)), Qt::QueuedConnection);
}

void KisAbstractSyndicationModel::removeFeed(const QString &feed)
{
    bool isRemoved = m_sites.removeOne(feed);
    if (isRemoved) {
        beginResetModel();
        QMutableListIterator<RssItem> it(m_aggregatedFeed);
        while (it.hasNext()) {
            RssItem item = it.next();
            if (item.source == feed)
                it.remove();
        }
        setArticleCount(m_aggregatedFeed.size());
        endResetModel();

        emit feedDataChanged();
    }
}

int KisAbstractSyndicationModel::rowCount(const QModelIndex &) const
{
    return m_aggregatedFeed.size();
}

QVariant KisAbstractSyndicationModel::data(const QModelIndex &index, int role) const
{

    RssItem item = m_aggregatedFeed.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    {
        QTextDocument doc;
        doc.setHtml(item.description);
        // Extract the first text block, which is the `<p>` element containing
        // the shortened post text, excluding the "This post [...] appeared
        // first on [...]" text.
        QString text = doc.firstBlock().text();
        if (text.length() > 92) {
            text.truncate(90);
            text.append("...");
        }
        return QString("<b><a href=\"" + item.link + "\">" + item.title + "</a></b>"
               "<br><small>(" + item.pubDate.toLocalTime().toString(Qt::DefaultLocaleShortDate) + ") "
               "<p style=\"margin-top: 4px\">" + text + "</p></small>");
    }
    case KisRssReader::RssRoles::TitleRole:
        return item.title;
    case KisRssReader::RssRoles::DescriptionRole:
        return item.description;
    case KisRssReader::RssRoles::PubDateRole:
        return item.pubDate.toString("dd-MM-yyyy hh:mm");
    case KisRssReader::RssRoles::LinkRole:
        return item.link;
    case KisRssReader::RssRoles::CategoryRole:
        return item.category;
    case KisRssReader::RssRoles::BlogNameRole:
        return item.blogName;
    case KisRssReader::RssRoles::BlogIconRole:
        return item.blogIcon;
    case KisRssReader::RssRoles::ThumbnailLinkRole:
        return item.thumbnailLink;
    }

    return QVariant();
}

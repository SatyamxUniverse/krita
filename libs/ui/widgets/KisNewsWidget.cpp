/*
 * Copyright (c) 2018 boud <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "KisNewsWidget.h"

#include <QDesktopServices>
#include <QUrl>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include "kis_config.h"
#include "KisMultiFeedRSSModel.h"
#include "QRegularExpression"


KisNewsDelegate::KisNewsDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void KisNewsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QStyleOptionViewItem optionCopy = option;
    initStyleOption(&optionCopy, index);

    QStyle *style = optionCopy.widget? optionCopy.widget->style() : QApplication::style();

    QTextDocument doc;
    doc.setHtml(optionCopy.text);
    doc.setDocumentMargin(10);

    /// Painting item without text
    optionCopy.text = QString();
    style->drawControl(QStyle::CE_ItemViewItem, &optionCopy, painter);

    QAbstractTextDocumentLayout::PaintContext ctx;

    // Highlighting text if item is selected
    if (optionCopy.state & QStyle::State_Selected) {
        ctx.palette.setColor(QPalette::Text, optionCopy.palette.color(QPalette::Active, QPalette::HighlightedText));
    }

    painter->translate(optionCopy.rect.left(), optionCopy.rect.top());
    QRect clip(0, 0, optionCopy.rect.width(), optionCopy.rect.height());
    doc.setPageSize(clip.size());
    doc.drawContents(painter, clip);
    painter->restore();
}

QSize KisNewsDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem optionCopy = option;
    initStyleOption(&optionCopy, index);

    QTextDocument doc;
    doc.setHtml(optionCopy.text);
    doc.setTextWidth(optionCopy.rect.width());
    return QSize(doc.idealWidth(), doc.size().height());
}

KisNewsWidget::KisNewsWidget(QWidget *parent)
    : QWidget(parent)
    , m_getNews(false)
    , m_rssModel(0)
    , m_needsVersionUpdate(false)
{
    setupUi(this);
    m_rssModel = new MultiFeedRssModel(this);
    connect(m_rssModel, SIGNAL(feedDataChanged()), this, SLOT(rssDataChanged()));


    setCursor(Qt::PointingHandCursor);

    listNews->setModel(m_rssModel);
    listNews->setItemDelegate(new KisNewsDelegate(listNews));
    connect(listNews, SIGNAL(clicked(QModelIndex)), this, SLOT(itemSelected(QModelIndex)));
}

void KisNewsWidget::setAnalyticsTracking(QString text)
{
    m_analyticsTrackingParameters = text;
}

bool KisNewsWidget::hasUpdateAvailable()
{
    return m_needsVersionUpdate;
}

QString KisNewsWidget::versionNumber()
{
    return m_newVersionNumber;
}

QString KisNewsWidget::versionLink()
{
    return m_newVersionLink;
}

void KisNewsWidget::toggleNews(bool toggle)
{
    KisConfig cfg(false);
    cfg.writeEntry<bool>("FetchNews", toggle);

    if (toggle) {
        m_rssModel->addFeed(QLatin1String("https://krita.org/en/feed/"));
    }
    else {
        m_rssModel->removeFeed(QLatin1String("https://krita.org/en/feed/"));
    }
}

void KisNewsWidget::itemSelected(const QModelIndex &idx)
{
    if (idx.isValid()) {
        QString link = idx.data(RssRoles::LinkRole).toString();

        // append query string for analytics tracking if we set it
        if (m_analyticsTrackingParameters != "") {

            // use title in analytics query string
            QString linkTitle = idx.data(RssRoles::TitleRole).toString();
            linkTitle = linkTitle.simplified(); // trims and makes 1 white space
            linkTitle = linkTitle.replace(" ", "");

            m_analyticsTrackingParameters = m_analyticsTrackingParameters.append(linkTitle);
            QDesktopServices::openUrl(QUrl(link.append(m_analyticsTrackingParameters)));

        } else {
            QDesktopServices::openUrl(QUrl(link));
        }


    }
}

void KisNewsWidget::rssDataChanged()
{

    // grab the latest release post and URL for reference later
    // if we need to update
    for (int i = 0; i < m_rssModel->rowCount(); i++)
    {
       const QModelIndex &idx = m_rssModel->index(i);

       if (idx.isValid()) {

           // only use official release announcements to get version number
           if ( idx.data(RssRoles::CategoryRole).toString() !=  "Official Release") {
               continue;
           }

           QString linkTitle = idx.data(RssRoles::TitleRole).toString();

           // regex to capture version number
           QRegularExpression versionRegex("\\d\\.\\d\\.?\\d?\\.?\\d");
           QRegularExpressionMatch matched = versionRegex.match(linkTitle);

           // only take the top match for release version since that is the newest
           if (matched.hasMatch()) {
               m_newVersionNumber = matched.captured(0);
               m_newVersionLink = idx.data(RssRoles::LinkRole).toString();
               break;
           }
       }
    }

    // see if we need to update our version, or we are on a dev version
    calculateVersionUpdateStatus();

    emit newsDataChanged();
}

void KisNewsWidget::calculateVersionUpdateStatus()
{
    // do nothing if we are in dev version.
    QString currentVersionString = qApp->applicationVersion();
    if (currentVersionString.contains("git")) {
        return;
    }

    QList<int> currentVersionParts;
    Q_FOREACH (QString number, currentVersionString.split(".")) {
        currentVersionParts.append(number.toInt());
    }

    QList<int> onlineReleaseAnnouncement;
    Q_FOREACH (QString number, m_newVersionNumber.split(".")) {
        onlineReleaseAnnouncement.append(number.toInt());
    }

    while (onlineReleaseAnnouncement.size() < 4) {
        onlineReleaseAnnouncement.append(0);
    }

    while (currentVersionParts.size() < 4) {
        currentVersionParts.append(0);
    }

    // Check versions from mayor to minor
    // We don't assume onlineRelease version is always equal or higher.
    bool makeUpdate = true;
    for (int i = 0; i <= 3; i++) {
        if (onlineReleaseAnnouncement.at(i) > currentVersionParts.at(i)) {
            m_needsVersionUpdate = (true & makeUpdate);
            return;
        } else if (onlineReleaseAnnouncement.at(i) < currentVersionParts.at(i)) {
            makeUpdate &= false;
        }
    }
}

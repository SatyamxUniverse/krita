#include "KisAtomFeedReader.h"

#include <QDomElement>
#include <QString>

RssItemList KisAtomFeedReader::parse(QNetworkReply *reply)
{
    RssItemList list;
    QXmlStreamReader streamReader(reply);

    while (!streamReader.atEnd()) {
        streamReader.readNext();
        if (streamReader.isStartElement() && streamReader.name() == "entry") {
            list.append(parseEntry(streamReader));
        }
    }
    return list;
}

RssItem KisAtomFeedReader::parseEntry(QXmlStreamReader &streamReader)
{
    RssItem item;
    while (!streamReader.atEnd()) {
        switch (streamReader.readNext()) {
        case QXmlStreamReader::StartElement: {
            const QXmlStreamAttributes attributes = streamReader.attributes();
            if (streamReader.name() == "link") {
                QString link = attributes.value("href").toString();
                if (link.isEmpty()) {
                    continue;
                }
                item.link = link;
            } else if (streamReader.prefix() == "media" && streamReader.name() == "thumbnail") {
                QString thumbnailLink = attributes.value("url").toString();
                if (thumbnailLink.isEmpty()) {
                    continue;
                }
                item.thumbnailLink = thumbnailLink;
            } else if (streamReader.prefix() == "media" && streamReader.name() == "title") {
                QString title = streamReader.readElementText();
                if (title.isEmpty()) {
                    continue;
                }
                item.title = title;
            } else if (streamReader.prefix() == "media" && streamReader.name() == "description") {
                QString description = streamReader.readElementText();
                if (description.isEmpty()) {
                    continue;
                }
                item.description = description;
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            if (streamReader.name() == "entry") {
                return item;
            }
        }
        default:
            break;
        }
    }
    return item;
}

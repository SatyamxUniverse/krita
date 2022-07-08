#ifndef __KISATOMFEEDREADER_H_
#define __KISATOMFEEDREADER_H_

#include <KisRssReader.h>
#include <QNetworkReply>
#include <QXmlStreamReader>

class KisAtomFeedReader
{
public:
    RssItemList parse(QNetworkReply *reply);

private:
    RssItem parseEntry(QXmlStreamReader &streamReader);
};

#endif // __KISATOMFEEDREADER_H_

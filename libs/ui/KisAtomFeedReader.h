#ifndef __KISATOMFEEDREADER_H_
#define __KISATOMFEEDREADER_H_

#include <KisRssReader.h>
#include <QXmlStreamReader>

class KisAtomFeedReader
{
public:
    RssItemList parse(QXmlStreamReader &streamReader);

private:
    RssItem parseEntry(QXmlStreamReader &streamReader);
};

#endif // __KISATOMFEEDREADER_H_

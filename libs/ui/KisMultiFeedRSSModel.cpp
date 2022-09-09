/**************************************************************************
**
** This file is part of Qt Creator
**
** SPDX-FileCopyrightText: 2011 Nokia Corporation and /or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** SPDX-License-Identifier: LGPL-2.1-only
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "KisMultiFeedRSSModel.h"

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
#include <KisAtomFeedReader.h>

MultiFeedRssModel::MultiFeedRssModel(QObject *parent)
    : KisAbstractSyndicationModel(parent)
{
}

MultiFeedRssModel::MultiFeedRssModel(KisNetworkAccessManager *nam, QObject *parent)
    : KisAbstractSyndicationModel(nam, parent)
{
}

MultiFeedRssModel::~MultiFeedRssModel()
{
    delete m_networkAccessManager;
}

RssItemList MultiFeedRssModel::parse(QNetworkReply *reply)
{
    KisRssReader reader;
    QXmlStreamReader streamReader(reply);
    return reader.parse(streamReader, reply->request().url().toString());
}

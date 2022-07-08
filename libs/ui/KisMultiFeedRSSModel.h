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

#ifndef MULTIFEEDRSSMODEL_H
#define MULTIFEEDRSSMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QDateTime>

#include <KisAbstractSyndicationModel.h>
#include <KisRssReader.h>

#include <kritaui_export.h>


class QNetworkReply;
class QNetworkAccessManager;
class KisNetworkAccessManager;

class KRITAUI_EXPORT MultiFeedRssModel : public KisAbstractSyndicationModel
{
    Q_OBJECT
public:
    explicit MultiFeedRssModel(QObject *parent = 0);
    explicit MultiFeedRssModel(KisNetworkAccessManager* nam, QObject *parent = 0);
    ~MultiFeedRssModel() override;

    RssItemList parse(QNetworkReply *reply) override;


private:

    friend class MockMultiFeedRssModel;
};


class KRITAUI_EXPORT KisAtomFeedModel : public KisAbstractSyndicationModel
{
    Q_OBJECT
public:
    explicit KisAtomFeedModel(QObject *parent = 0);
    explicit KisAtomFeedModel(KisNetworkAccessManager* nam, QObject *parent = 0);
    ~KisAtomFeedModel() override;

    RssItemList parse(QNetworkReply *reply) override;

private:

    friend class MockMultiFeedRssModel;
};

#endif // MULTIFEEDRSSMODEL_H



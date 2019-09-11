/*
 *  Copyright (c) 2009 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef _PSD_CONVERTER_H_
#define _PSD_CONVERTER_H_

#include <stdio.h>

#include <QObject>
#include <QPointer>
#include <QFileInfo>

#include "kis_types.h"
#include <KisImportExportErrorCode.h>


// max number of pixels in one dimension of psd file
extern const int MAX_PSD_SIZE;


class KisDocument;
class KoUpdater;

class PSDSaver : public QObject {

    Q_OBJECT

public:

    PSDSaver(KisDocument *doc);
    PSDSaver(KisDocument *doc, QPointer<KoUpdater> updater);
    ~PSDSaver() override;

public:

    KisImportExportErrorCode buildFile(QIODevice *io);

    KisImageSP image();

public Q_SLOTS:

    virtual void cancel();

private:
    void setProgress(int progress);

private:
    KisImageSP m_image;
    KisDocument *m_doc;
    QPointer<KoUpdater> m_updater;
    bool m_stop;
};

#endif

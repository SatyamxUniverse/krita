/*
 *  SPDX-FileCopyrightText: 2013 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_UPDATE_OUTLINE_JOB_H
#define __KIS_UPDATE_OUTLINE_JOB_H

#include <QColor>

#include "kis_spontaneous_job.h"
#include "kis_selection.h"

class KRITAIMAGE_EXPORT KisUpdateOutlineJob : public KisSpontaneousJob
{
public:
    KisUpdateOutlineJob(KisSelectionSP selection, bool updateThumbnail, const QColor &maskColor1, const QColor &maskColor2);

    bool overrides(const KisSpontaneousJob *otherJob) override;
    void run() override;
    int levelOfDetail() const override;
    QString debugName() const override;

private:
    KisSelectionSP m_selection;
    bool m_updateThumbnail;
    QColor m_maskColor1;
    QColor m_maskColor2;
};

#endif /* __KIS_UPDATE_OUTLINE_JOB_H */

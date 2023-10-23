/*
 *  SPDX-FileCopyrightText: 2013 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_update_outline_job.h"


KisUpdateOutlineJob::KisUpdateOutlineJob(KisSelectionSP selection, bool updateThumbnail, const QColor &maskColor1, const QColor &maskColor2)
    : m_selection(selection),
      m_updateThumbnail(updateThumbnail),
      m_maskColor1(maskColor1),
      m_maskColor2(maskColor2)
{
}

bool KisUpdateOutlineJob::overrides(const KisSpontaneousJob *otherJob)
{
    return dynamic_cast<const KisUpdateOutlineJob*>(otherJob);
}

void KisUpdateOutlineJob::run()
{
    m_selection->recalculateOutlineCache();
    if (m_updateThumbnail) {
        m_selection->recalculateThumbnailImage(m_maskColor1, m_maskColor2);
    }
    m_selection->notifySelectionChanged();
}

int KisUpdateOutlineJob::levelOfDetail() const
{
    return 0;
}

QString KisUpdateOutlineJob::debugName() const
{
    return "KisUpdateOutlineJob";
}

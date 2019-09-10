/*
 *  Copyright (c) 2019 Agata Cacko <cacko.azh@gmail.com>
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
#ifndef KISUPDATERWRAPPERSUBTASK_H
#define KISUPDATERWRAPPERSUBTASK_H

#include <QPointer>
#include <kritaui_export.h>


class KisUpdaterWrapper;

/**
 * @brief The KisUpdaterWrapperSubtask class is used with KisUpdaterWrapper for easier calculation of progress for subtasks.
 *
 * This class represents one subtask. When the subtask progresses, setProgress()
 *    function should be called to inform the wrapper about new progress.
 */
class KRITAUI_EXPORT KisUpdaterWrapperSubtask
{
public:
    /**
     * @brief KisUpdaterWrapperSubtask constructor
     * @param wrapper wrapper pointer
     * @param weight weight of the subtask
     */
    KisUpdaterWrapperSubtask(QPointer<KisUpdaterWrapper> wrapper, int weight);

    /**
     * @brief setProgress function is called when the subtask progress changed
     * @param progress made by this subtask
     */
    void setProgress(int progress);

    /**
     * @brief m_wrapper pointer to the wrapper with all subtasks
     */
    QPointer<KisUpdaterWrapper> m_wrapper;
    /**
     * @brief m_id id of the subtask in the wrapper
     */
    int m_id;
};

#endif // KISUPDATERWRAPPERSUBTASK_H

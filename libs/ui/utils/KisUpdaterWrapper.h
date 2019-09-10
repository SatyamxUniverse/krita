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
#ifndef KISUPDATERWRAPPER_H
#define KISUPDATERWRAPPER_H

#include <QPointer>
#include <QList>
#include <QObject>

#include <kritaui_export.h>

class KisUpdaterWrapperSubtask;
class KoUpdater;


/**
 * This class helps calculating progress
 *    when there are a few distinct subtasks
 *    within the same thread.
 *
 * This class is meant to be a replacement
 *    for KisProgressUpdater, but the usage is a bit
 *    different. This class is also more limited:
 *    it is *not* thread-safe and is not meant to be,
 *    it doesn't use signals so it's more robust,
 *    basically it's simpler, but less error-prone.
 *
 * Usage:
 *  - create one KisUpdaterWrapper.
 *  - create how many subtasks as you wish with this wrapper
 *      as a constructor argument. It will automatically
 *      register them into the wrapper.
 *  - use subtasks to set progress.
 *
 * Notes:
 *  - *don't* make multiple KisUpdaterWrappers for one updater!
 *  - *don't* set progress directly onto the updater!
 *      If you don't know how to set the remaining x%,
 *      create a new subtask and use that. All progress
 *      needs to be set using a subtask.
 *  - subtasks' progress is 0-100 based, just like the updater
 *  - it should be possible to use with a KisProgressUpdater -
 *      as long as KisUpdaterWrapper is on the lowest level.
 *      It should also be possible to make a wrapper
 *      (inherited class) for an updater to work as a subtask
 *      for this wrapper (it just needs to work just like
 *      KisUpdaterWrapper). Note that in time of writing
 *      KisProgressUpdater is not working correctly
 *      inside threads without an event loop.
 */
class KRITAUI_EXPORT KisUpdaterWrapper : public QObject
{
public:
    /**
     * @brief The SubtaskInformation struct is used to store information about subtasks
     */
    struct SubtaskInformation {
        /**
         * @brief weight weight of the subtask
         */
        int weight;
        /**
         * @brief currentSum current progress of the subtask
         */
        int currentProgress;
    };

public:
    /**
     * @brief KisUpdaterWrapper constructor
     * @param updater main updater that would be filles with progress
     *
     * This constructor just prepares the class to be used.
     */
    KisUpdaterWrapper(QPointer<KoUpdater> updater);

    /**
     * @brief registerSubtask function is used to add subtask to the wrapper
     * @param weight weight of the subtask
     * @return id of the subtask (used later to distinguish subtasks)
     */
    int registerSubtask(int weight);
    /**
     * @brief setProgress function is used to set partial progress made the subtask
     * @param id id of the subtask
     * @param progress progress made by this one subtask
     */
    void setProgress(int id, int progress);

    /**
     * @brief m_mainUpdater main updater, the one that this class sets progress to
     */
    QPointer<KoUpdater> m_mainUpdater;
    /**
     * @brief m_subtasksList list of registered subtasks
     *
     * All subtasks have an id, this id is an index in this list.
     */
    QList<SubtaskInformation> m_subtasksList;
    /**
     * @brief m_weightsSum sum of weights of all registered subtasks
     */
    int m_weightsSum;
    /**
     * @brief m_lastProgress last seen value of summed up progress (progress for all subtasks)
     */
    int m_lastProgress;

};

#endif // KISUPDATERWRAPPER_H

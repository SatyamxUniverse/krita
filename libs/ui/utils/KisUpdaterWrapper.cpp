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
#include <KisUpdaterWrapper.h>
#include <KoUpdater.h>

KisUpdaterWrapper::KisUpdaterWrapper(QPointer<KoUpdater> updater)
    : m_mainUpdater(updater)
    , m_subtasksList()
    , m_weightsSum(0)
    , m_lastProgress(0)
{

}

int KisUpdaterWrapper::registerSubtask(int weight)
{
    int id = m_subtasksList.count();

    SubtaskInformation info = SubtaskInformation();
    info.weight = weight;
    info.currentProgress = 0;
    m_subtasksList.append(info);

    m_weightsSum += weight;

    return id;
}

void KisUpdaterWrapper::setProgress(int id, int progress)
{
    if (m_subtasksList[id].currentProgress == progress) {
        return; // let's not recalculate in this case
    }
    m_subtasksList[id].currentProgress = progress;


    int sum = 0;
    Q_FOREACH(struct SubtaskInformation info, m_subtasksList) {
        sum += info.currentProgress*info.weight;
    }
    sum /= m_weightsSum;
    if (m_lastProgress != sum) {
        m_mainUpdater->setProgress(m_lastProgress);
    }
    m_lastProgress = sum;
}



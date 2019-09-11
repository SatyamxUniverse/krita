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
#include <KisUpdaterWrapperSubtask.h>
#include <KisUpdaterWrapper.h>

KisUpdaterWrapperSubtask::KisUpdaterWrapperSubtask(QPointer<KisUpdaterWrapper> wrapper, int weight)
    : m_wrapper(wrapper)
    , m_stepsNumberMax(100)
    , m_stepsNumberCurrent(0)
{
    m_id = wrapper->registerSubtask(weight);
}


void KisUpdaterWrapperSubtask::setProgress(int progress)
{
    m_wrapper->setProgress(m_id, progress);
}

void KisUpdaterWrapperSubtask::setStepsNumber(int stepsNumber)
{
    m_stepsNumberMax = stepsNumber;
}

void KisUpdaterWrapperSubtask::nextStep()
{
    m_stepsNumberCurrent += 1;
    setProgress((100*m_stepsNumberCurrent)/m_stepsNumberMax);
}

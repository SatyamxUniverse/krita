/*
 *  SPDX-FileCopyrightText: 2023 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef MULTITHREADEDFILLTESTHELPERS_H
#define MULTITHREADEDFILLTESTHELPERS_H

#include "kis_image.h"
#include "KisRunnableBasedStrokeStrategy.h"

namespace TestUtil
{
void runFillStroke(KisImageSP image, std::function<void(KisRunnableStrokeJobsInterface*)> functor)
{
    struct FillStroke : public KisRunnableBasedStrokeStrategy {
        FillStroke(std::function<void(KisRunnableStrokeJobsInterface*)> functor)
            : KisRunnableBasedStrokeStrategy(QLatin1String("test-fill-stroke"), kundo2_noi18n("test-fill-stroke"))
            , m_functor(functor)
        {
            this->enableJob(JOB_INIT);
            this->enableJob(JOB_DOSTROKE);
        }

        void initStrokeCallback() override {
            m_functor(runnableJobsInterface());
        }

    private:
        std::function<void(KisRunnableStrokeJobsInterface*)> m_functor;
    };


    KisStrokeId id = image->startStroke(new FillStroke(functor));
    image->endStroke(id);
    image->waitForDone();
}
}

#endif // MULTITHREADEDFILLTESTHELPERS_H

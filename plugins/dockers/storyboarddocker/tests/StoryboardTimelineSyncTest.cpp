/*
 *  SPDX-FileCopyrightText: 2020 Saurabh Kumar <saurabhk660@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "StoryboardTimelineSyncTest.h"
#include "StoryboardModel.h"
#include "StoryboardView.h"

#include <kis_group_layer.h>

#include <simpletest.h>


void StoryboardTimelineSyncTest::initTestCase()
{
    m_image = new KisImage(0, 100, 100, nullptr, "image");

    m_storyboardModel = new StoryboardModel(this);
    m_storyboardView = new StoryboardView();
    m_storyboardModel->setView(m_storyboardView);
    m_storyboardView->setModel(m_storyboardModel);
    m_storyboardModel->setImage(m_image);

    m_layer1 = new KisPaintLayer(m_image, "layer1", OPACITY_OPAQUE_U8);
    m_image->addNode(m_layer1, m_image->rootLayer());

    m_layer1->enableAnimation();
    m_channel1 = m_layer1->getKeyframeChannel(KisKeyframeChannel::Raster.id(), true);

    m_layer2 = new KisPaintLayer(m_image, "layer2", OPACITY_OPAQUE_U8);
    m_image->addNode(m_layer2, m_image->rootLayer());

    m_layer2->enableAnimation();
    m_channel2 = m_layer2->getKeyframeChannel(KisKeyframeChannel::Raster.id(), true);

    m_channel1->addKeyframe(0);
    m_channel2->addKeyframe(0);

    QCOMPARE(m_storyboardModel->rowCount(), 0);
}

void StoryboardTimelineSyncTest::cleanupTestCase()
{
    m_image->waitForDone();

    delete m_storyboardModel;
    delete m_storyboardView;
}

void StoryboardTimelineSyncTest::cleanup()
{
    //testStoryboardItemSortedUniquePositive();

    const int priorChannel1Count = m_channel1->keyframeCount();
    const int priorChannel2Count = m_channel2->keyframeCount();

    m_storyboardModel->removeRows(0, m_storyboardModel->rowCount());

    QCOMPARE(m_channel1->keyframeCount(), priorChannel1Count);
    QCOMPARE(m_channel2->keyframeCount(), priorChannel2Count);
    QCOMPARE(m_storyboardModel->rowCount(), 0);
}

void StoryboardTimelineSyncTest::testAddKeyframeExtendsDuration()
{
    m_storyboardModel->insertRow(0);
    m_storyboardModel->insertChildRows(0);
    QModelIndex item1Index = m_storyboardModel->index(0, 0);

    QVERIFY(item1Index.isValid());
    QCOMPARE(m_storyboardModel->rowCount(), 1);

    //Adding a keyframe beyond the last scene (atm starting on frame 0)
    //should extend the length of the last scene to encompass new
    //keyframe entries.

    m_channel1->addKeyframe(5);

    {
        // Scenes == [...]
        // [0 1 2 3 4 5] 6 7 8 ...
        //Scene duration should be 0s, 6f
        //Remember that first frame is included as 1 whole frame!
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, item1Index);
        QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, item1Index);
        QCOMPARE(frameDurationIndex.data().toInt(), 6);
        QCOMPARE(secondDurationIndex.data().toInt(), 0);
    }

    m_channel1->moveKeyframe(5, 25);

    {
        // [0 1 2 ... 23 24 25] 26 27 28 ...
        //Scene duration should be 1s, 1f
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, item1Index);
        QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, item1Index);
        QCOMPARE(frameDurationIndex.data().toInt(), 2);
        QCOMPARE(secondDurationIndex.data().toInt(), 1);
        QCOMPARE(m_channel1->keyframeCount(), 2);
    }

    m_storyboardModel->slotSetActiveNode(m_layer1);
    m_storyboardModel->insertItem(item1Index, true);
    QCOMPARE(m_storyboardModel->rowCount(), 2);
    QVERIFY(m_channel1->keyframeAt(26));
    QVERIFY(m_channel2->keyframeAt(26));
    QModelIndex item2Index = m_storyboardModel->index(1,0);

    {
        // [0 1 2 ... 23 24 25] [26] 27 28 29 ...
        QModelIndex frameNumberIndex = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, item2Index);
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, item2Index);
        QCOMPARE(frameNumberIndex.data().toInt(), 26);
        QCOMPARE(frameDurationIndex.data().toInt(), 1);
    }

    m_channel2->addKeyframe(28);
    QVERIFY(m_channel2->keyframeCount() == 3);
    QVERIFY(m_channel1->keyframeCount() == 3);

    {
        // [0 1 2 ... 23 24 25] [26 27 28] 29 30 31 ...
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, item2Index);
        QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, item2Index);
        QCOMPARE(frameDurationIndex.data().toInt(), 3);
        QCOMPARE(secondDurationIndex.data().toInt(), 0);
    }


    m_channel2->removeKeyframe(28);
    m_channel2->removeKeyframe(26);
    m_channel1->removeKeyframe(26);
    m_channel1->removeKeyframe(25);

    QVERIFY(m_storyboardModel->rowCount() == 1);
    QCOMPARE(m_channel2->keyframeCount(), 1);
    QCOMPARE(m_channel1->keyframeCount(), 1);

    m_storyboardModel->removeRows(0, 1);

    QVERIFY(m_storyboardModel->rowCount() == 0);
}

void StoryboardTimelineSyncTest::testStoryboardToTimelineSyncronization()
{
    //             0  1  2  3  4  5  6  7  8  9
    //  channel1  [|  .  |] [| .] .  .  .  .  .
    //  channel2  [|  .  |] [| |] .  .  .  .  .

    { //SETUP
        m_storyboardModel->insertItem(m_storyboardModel->index(0,0), true);
        QCOMPARE(m_storyboardModel->rowCount(), 1);
        m_channel1->addKeyframe(2);
        m_channel2->addKeyframe(2);

        m_storyboardModel->insertItem(m_storyboardModel->index(0,0), true);
        m_channel2->addKeyframe(4);
        QCOMPARE(m_storyboardModel->rowCount(), 2);

        QCOMPARE(m_channel1->keyframeCount(), 3);
        QCOMPARE(m_channel2->keyframeCount(), 4);
    }


    QSignalSpy spyTimeChanged(m_image->animationInterface() , SIGNAL(sigUiTimeChanged(int)));
    QVERIFY(spyTimeChanged.isValid());

    m_image->animationInterface()->switchCurrentTimeAsync(0);
    QCOMPARE(spyTimeChanged.count(), 1);

    QModelIndex parentIndex = m_storyboardView->currentIndex();
    QVERIFY(parentIndex.isValid());
    QCOMPARE(m_storyboardModel->index(StoryboardItem::FrameNumber, 0, parentIndex).data().toInt(), 0);

    m_image->animationInterface()->switchCurrentTimeAsync(2);
    parentIndex = m_storyboardView->selectionModel()->currentIndex();
    QCOMPARE(m_storyboardModel->index(StoryboardItem::FrameNumber, 0, parentIndex).data().toInt(), 0);

    m_image->animationInterface()->switchCurrentTimeAsync(3);
    parentIndex = m_storyboardView->selectionModel()->currentIndex();
    QCOMPARE(m_storyboardModel->index(StoryboardItem::FrameNumber, 0, parentIndex).data().toInt(), 3);

    { // CLEANUP
        m_storyboardModel->removeRows(0, m_storyboardModel->rowCount());
        QCOMPARE(m_storyboardModel->rowCount(), 0);

        m_channel2->removeKeyframe(4);
        m_channel2->removeKeyframe(3);
        m_channel2->removeKeyframe(2);
        m_channel1->removeKeyframe(3);
        m_channel1->removeKeyframe(2);
        QCOMPARE(m_channel1->keyframeCount(), 1);
        QCOMPARE(m_channel2->keyframeCount(), 1);
    }
}

void StoryboardTimelineSyncTest::testTimelineToStoryboardSyncronization()
{
    //             0  ... 29  30 ... 41 ... 59  60 ... 89
    //  channel1  [|  ... |] [| ...  |  ... .] [| ... .] .  .  .
    //  channel2  [|  ... |] [| ...  .  ... .] [| ... .] .  .  .

    { //SETUP
        m_storyboardModel->insertItem(m_storyboardModel->index(0,0), true);
        QCOMPARE(m_storyboardModel->rowCount(), 1);
        m_channel1->addKeyframe(29);
        m_channel2->addKeyframe(29);

        m_storyboardModel->insertItem(m_storyboardModel->index(0,0), true);
        m_channel1->addKeyframe(41);
        QCOMPARE(m_storyboardModel->rowCount(), 2);

        {
            QModelIndex itemIndex = m_storyboardModel->index(1, 0);
            QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, itemIndex);
            QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, itemIndex);
            m_storyboardModel->setData(secondDurationIndex, 1);
            m_storyboardModel->setData(frameDurationIndex, 6);
            QCOMPARE(secondDurationIndex.data().toInt(), 1);
            QCOMPARE(frameDurationIndex.data().toInt(), 6);
        }

        m_storyboardModel->insertItem(m_storyboardModel->index(1,0), true);
        QCOMPARE(m_storyboardModel->rowCount(), 3);

        {
            QModelIndex itemIndex = m_storyboardModel->index(2, 0);
            QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, itemIndex);
            QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, itemIndex);
            m_storyboardModel->setData(secondDurationIndex, 1);
            m_storyboardModel->setData(frameDurationIndex, 6);
            QCOMPARE(secondDurationIndex.data().toInt(), 1);
            QCOMPARE(frameDurationIndex.data().toInt(), 6);
        }

        QCOMPARE(m_channel1->keyframeCount(), 5);
        QCOMPARE(m_channel2->keyframeCount(), 4);


    }

    {   // Move all frames 0 > 1
        // No new scenes should be created,
        // first scene is *implicit* and cannot
        // be moved via the timeline since a
        // keyframe 0 always exists.
        const int originalRowCount = m_storyboardModel->rowCount();

        KUndo2Command undo;
        m_channel1->moveKeyframe(0, 1, &undo);
        m_channel2->moveKeyframe(0, 1, &undo);

        QCOMPARE(m_storyboardModel->rowCount(), originalRowCount);
        QCOMPARE(m_channel1->keyframeCount(), 6);
        QCOMPARE(m_channel2->keyframeCount(), 5);

        undo.undoMergedCommands();

        QCOMPARE(m_storyboardModel->rowCount(), originalRowCount);
        QCOMPARE(m_channel1->keyframeCount(), 5);
        QCOMPARE(m_channel2->keyframeCount(), 4);
    }


    {   // Move chan 1 frames 30 > 31,
        // Since only 1 layer is changed,
        // No time adjustment should be made to storyboard

        // Setup..
        QModelIndex subjectIndex = m_storyboardModel->index(1, 0);
        const int subjectOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

        const int subjectOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

        QModelIndex prevIndex = m_storyboardModel->index(0, 0);
        const int prevOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

        const int prevOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

        // Verify integrity..
        QCOMPARE(subjectOriginalDurationInFrames, 30);
        QCOMPARE(subjectOriginalStartingFrame, 30);
        QCOMPARE(prevOriginalDurationInFrames, 30);
        QCOMPARE(prevOriginalStartingFrame, 0);

        KUndo2Command undo;
        m_channel1->moveKeyframe(30, 31, &undo);


        {
            // No changes should be made before or after undo..
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame);

            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }

        undo.undoMergedCommands();


        {
            // No changes should be made before or after undo..
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame);

            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);

            QCOMPARE(m_channel1->keyframeCount(), 5);
            QCOMPARE(m_channel2->keyframeCount(), 4);
        }
    }

    {   // Move chan 1 frames 30 > 28
        // This should also do effectively nothing
        // since there are still keys that exist in
        // scene #2..

        // Setup..
        QModelIndex prevIndex = m_storyboardModel->index(0, 0);
        const int prevOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

        const int prevOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

        QModelIndex subjectIndex = m_storyboardModel->index(1, 0);
        const int subjectOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

        const int subjectOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

        const int originalStoryboardCount = m_storyboardModel->rowCount();

        // Verify integrity..
        QCOMPARE(originalStoryboardCount, 3);
        QCOMPARE(prevOriginalDurationInFrames, 30);
        QCOMPARE(prevOriginalStartingFrame, 0);

        KUndo2Command undo;
        m_channel1->moveKeyframe(30, 28, &undo);

        {
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be the same before and after undo..

            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount);
            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame);
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }

        undo.undoMergedCommands();

        {
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount);
            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame);
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }

        QCOMPARE(m_channel1->keyframeCount(), 5);
        QCOMPARE(m_channel2->keyframeCount(), 4);
    }

    {   // Move all frames 30 > 31,
        // Should move within scope of scene and
        // thus change the start frame and
        // and previous scene's duration.

        // Setup..
        QModelIndex subjectIndex = m_storyboardModel->index(1, 0);
        const int subjectOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

        const int subjectOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

        QModelIndex prevIndex = m_storyboardModel->index(0, 0);
        const int prevOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

        const int prevOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

        // Verify integrity..
        QCOMPARE(subjectOriginalDurationInFrames, 30);
        QCOMPARE(subjectOriginalStartingFrame, 30);
        QCOMPARE(prevOriginalDurationInFrames, 30);
        QCOMPARE(prevOriginalStartingFrame, 0);

        KUndo2Command undo;
        m_channel1->moveKeyframe(30, 31, &undo);
        m_channel2->moveKeyframe(30, 31, &undo);

        {
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be offset by 1 frame with 1 less frame duration.
            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames - 1);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame + 1);

            //Should be 1 frame longer.
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames + 1);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }

        // Undo should work and revert storyboard information to what it was before!
        undo.undoMergedCommands();


        {
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be the same..
            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame);
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }
    }

    {   // Move all 30 > 28
        // This should merge scene 2 with scene 1
        // by extending scene length of 1 and removing
        // scene 2

        // Setup..
        QModelIndex prevIndex = m_storyboardModel->index(0, 0);
        const int prevSceneOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

        const int prevSceneOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

        QModelIndex subjectIndex = m_storyboardModel->index(1, 0);
        const int subjectOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

        const int currSceneOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

        const int originalStoryboardCount = m_storyboardModel->rowCount();

        // Verify integrity..
        QCOMPARE(originalStoryboardCount, 3);
        QCOMPARE(prevSceneOriginalDurationInFrames, 30);
        QCOMPARE(prevSceneOriginalStartingFrame, 0);

        KUndo2Command undo;
        m_channel1->moveKeyframe(30, 28, &undo);
        m_channel1->moveKeyframe(41, 26, &undo);
        m_channel2->moveKeyframe(30, 28, &undo);

        {
            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be offset by 1 frame with 1 less frame duration.
            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount - 1);

            //Should be 2 frames longer.
            QCOMPARE(prevDurationInFrames, prevSceneOriginalDurationInFrames + subjectOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevSceneOriginalStartingFrame);
        }

        // Must also be undone correctly..
        undo.undoMergedCommands();

        {
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

//            //Should be the same..
            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount);
            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, currSceneOriginalStartingFrame);
            QCOMPARE(prevDurationInFrames, prevSceneOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevSceneOriginalStartingFrame);
        }
    }


    {   // Move all 30 > 61 and 41 > 62
        // This should also merge scene 2 down to
        // scene 1.

        // Setup..
        QModelIndex prevIndex = m_storyboardModel->index(0, 0);
        const int prevOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

        const int prevOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

        QModelIndex subjectIndex = m_storyboardModel->index(1, 0);
        const int subjectOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

        const int subjectOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

        const int originalStoryboardCount = m_storyboardModel->rowCount();

        // Verify integrity..
        QCOMPARE(originalStoryboardCount, 3);
        QCOMPARE(prevOriginalDurationInFrames, 30);
        QCOMPARE(prevOriginalStartingFrame, 0);

        KUndo2Command undo;
        m_channel1->moveKeyframe(30, 61, &undo);
        m_channel1->moveKeyframe(41, 62, &undo);
        m_channel2->moveKeyframe(30, 61, &undo);

        {
            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be offset by 1 frame with 1 less frame duration.
            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount - 1);

            //Should be 2 frames longer.
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames + subjectOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }

        // Must also be undone correctly..
        undo.undoMergedCommands();

        {
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be the same..
            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount);
            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame);
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }
    }

    {   // Delete all 30 and 41
        // This should also merge scene 2 into
        // scene 1.

        ENTER_FUNCTION() << "DELETE BEGIN";

        // Setup..
        QModelIndex prevIndex = m_storyboardModel->index(0, 0);
        const int prevOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

        const int prevOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

        QModelIndex subjectIndex = m_storyboardModel->index(1, 0);
        const int subjectOriginalDurationInFrames = m_image->animationInterface()->framerate() *
                m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

        const int subjectOriginalStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

        const int originalStoryboardCount = m_storyboardModel->rowCount();

        // Verify integrity..
        QCOMPARE(originalStoryboardCount, 3);
        QCOMPARE(prevOriginalDurationInFrames, 30);
        QCOMPARE(prevOriginalStartingFrame, 0);

        KUndo2Command undo;
        m_channel1->removeKeyframe(30, &undo);
        m_channel1->removeKeyframe(41, &undo);
        m_channel2->removeKeyframe(30, &undo);

        {
            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be offset by 1 frame with 1 less frame duration.
            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount - 1);

            //Should be 2 frames longer.
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames + subjectOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }

        // Must also be undone correctly..
        undo.undoMergedCommands();

        {
            const int subjectDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, subjectIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, subjectIndex).data().toInt();

            const int subjectStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, subjectIndex).data().toInt();

            const int prevDurationInFrames = m_image->animationInterface()->framerate() *
                    m_storyboardModel->index(StoryboardItem::DurationSecond, 0, prevIndex).data().toInt() +
                    m_storyboardModel->index(StoryboardItem::DurationFrame, 0, prevIndex).data().toInt();

            const int prevStartingFrame = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, prevIndex).data().toInt();

            //Should be the same..
            QCOMPARE(m_storyboardModel->rowCount(), originalStoryboardCount);
            QCOMPARE(subjectDurationInFrames, subjectOriginalDurationInFrames);
            QCOMPARE(subjectStartingFrame, subjectOriginalStartingFrame);
            QCOMPARE(prevDurationInFrames, prevOriginalDurationInFrames);
            QCOMPARE(prevStartingFrame, prevOriginalStartingFrame);
        }

        ENTER_FUNCTION() << "DELETE END";
    }


    { // CLEANUP
        ENTER_FUNCTION() << "CLEANUP BEGIN";
        m_storyboardModel->removeRows(0, m_storyboardModel->rowCount());

        m_channel2->removeKeyframe(60);
        m_channel2->removeKeyframe(30);
        m_channel2->removeKeyframe(29);

        m_channel1->removeKeyframe(60);
        m_channel1->removeKeyframe(30);
        m_channel1->removeKeyframe(41);
        m_channel1->removeKeyframe(29);

        QCOMPARE(m_channel1->keyframeCount(), 1);
        QCOMPARE(m_channel2->keyframeCount(), 1);
    }
}


void StoryboardTimelineSyncTest::testDurationChange()
{
    const int fps = m_image->animationInterface()->framerate();

    m_storyboardModel->insertRow(0);
    m_storyboardModel->insertChildRows(0);
    m_channel1->addKeyframe(3);
    m_storyboardModel->slotSetActiveNode(m_layer1);
    m_storyboardModel->insertItem(m_storyboardModel->index(0,0), true);
    QCOMPARE(m_channel1->keyframeCount(), 3);
    QVERIFY(m_channel1->keyframeAt(4));
    QCOMPARE(m_storyboardModel->rowCount(), 2);

    {
        // [0 1 2 3] [4] 5 6 7 ..
        // Verify the above ^
        QModelIndex itemIndex = m_storyboardModel->index(0, 0);
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, itemIndex);
        QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, itemIndex);
        QCOMPARE(frameDurationIndex.data().toInt(), 4);
        QCOMPARE(secondDurationIndex.data().toInt(), 0);

        itemIndex = m_storyboardModel->index(1, 0);
        frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, itemIndex);
        secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, itemIndex);
        QCOMPARE(frameDurationIndex.data().toInt(), 1);
        QCOMPARE(secondDurationIndex.data().toInt(), 0);
    }

    {   // Try setting scene length too short. Scenes can never be shorter than their "internal" keyframe contents.
        // In this case, the scene length cannot be less than or equal to 3!
        QModelIndex parentIndex = m_storyboardModel->index(0,0);
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, parentIndex);
        m_storyboardModel->setData(frameDurationIndex, 3);
        QCOMPARE(frameDurationIndex.data().toInt(), 4);
    }

    {   // Try setting scene length 1 frame longer.
        // In this case, the next scene should start at frame 5 and keyframes should reflect this change!
        QModelIndex parentIndex = m_storyboardModel->index(0,0);
        QModelIndex nextParentIndex = m_storyboardModel->index(1,0);
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, parentIndex);
        QModelIndex nextFrameNumberIndex = m_storyboardModel->index(StoryboardItem::FrameNumber, 0, nextParentIndex);
        m_storyboardModel->setData(frameDurationIndex, 5);
        QCOMPARE(frameDurationIndex.data().toInt(), 5);
        QCOMPARE(nextFrameNumberIndex.data().toInt(), 5);
        QVERIFY(m_channel1->keyframeAt(5));
    }

    {   // Try setting scene length 3 frames higher than fps count.
        // Should be 1s, 3f
        QModelIndex parentIndex = m_storyboardModel->index(0,0);
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, parentIndex);
        QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, parentIndex);
        m_storyboardModel->setData(frameDurationIndex, fps + 3);
        QCOMPARE(frameDurationIndex.data().toInt(), 3);
        QCOMPARE(secondDurationIndex.data().toInt(), 1);
        QVERIFY(m_channel1->keyframeAt(fps + 3));
    }

    {   // Try setting scene length to zero seconds.
        // Will try be 0s, 3f; However, it will be restricted to lowest possible length (0s, 4f)
        // as described above...
        QModelIndex parentIndex = m_storyboardModel->index(0,0);
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, parentIndex);
        QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, parentIndex);
        m_storyboardModel->setData(secondDurationIndex, 0);
        QCOMPARE(frameDurationIndex.data().toInt(), 4);
        QCOMPARE(secondDurationIndex.data().toInt(), 0);
        QVERIFY(m_channel1->keyframeAt(4));
    }

    { // Cleanup time..
        m_storyboardModel->removeRows(0, m_storyboardModel->rowCount());

        Q_FOREACH( const int& keyframeTime, m_channel1->allKeyframeTimes()) {
            if (keyframeTime == 0) {
                continue;
            }

            m_channel1->removeKeyframe(keyframeTime);
        }

        Q_FOREACH( const int& keyframeTime, m_channel2->allKeyframeTimes()) {
            if (keyframeTime == 0) {
                continue;
            }

            m_channel2->removeKeyframe(keyframeTime);
        }

        QCOMPARE(m_storyboardModel->rowCount(), 0);
        QCOMPARE(m_channel1->keyframeCount(), 1);
    }
}


void StoryboardTimelineSyncTest::testFpsChanged()
{
    const int originalFPS = m_image->animationInterface()->framerate();

    m_storyboardModel->insertRow(0);
    m_storyboardModel->insertChildRows(0);
    m_storyboardModel->slotSetActiveNode(m_layer1);
    m_storyboardModel->insertItem(m_storyboardModel->index(0,0), true);
    QCOMPARE(m_channel1->keyframeCount(), 2);
    QVERIFY(m_channel1->keyframeAt(1));
    QCOMPARE(m_storyboardModel->rowCount(), 2);

    { // Let's set the length of the first scene to 30 frames. Should be 1s, 6f
        QModelIndex parentIndex = m_storyboardModel->index(0,0);
        QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, parentIndex);
        QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, parentIndex);
        m_storyboardModel->setData(frameDurationIndex, 30);
        QCOMPARE(frameDurationIndex.data().toInt(), 6);
        QCOMPARE(secondDurationIndex.data().toInt(), 1);
        QVERIFY(m_channel1->keyframeAt(30));
    }

    for( int i = 0; i <= 30; i += 10) {
        const int newFPS = qMax(i, 1);
        m_image->animationInterface()->setFramerate(newFPS);

        { // Let's check the length of the first scene to make sure it has been updated to match newFPS.
            QModelIndex parentIndex = m_storyboardModel->index(0,0);
            QModelIndex frameDurationIndex = m_storyboardModel->index(StoryboardItem::DurationFrame, 0, parentIndex);
            QModelIndex secondDurationIndex = m_storyboardModel->index(StoryboardItem::DurationSecond, 0, parentIndex);
            QCOMPARE(frameDurationIndex.data().toInt(), 30 % newFPS);
            QCOMPARE(secondDurationIndex.data().toInt(), 30 / newFPS);
            QVERIFY(m_channel1->keyframeAt(30));
        }
    }

    {   // Cleanup time
        m_image->animationInterface()->setFramerate(originalFPS);

        m_storyboardModel->removeRows(0, m_storyboardModel->rowCount());
        m_channel1->removeKeyframe(30);

        QCOMPARE( m_storyboardModel->rowCount(), 0);
        QCOMPARE( m_channel1->keyframeCount(), 1);
    }
}

SIMPLE_TEST_MAIN(StoryboardTimelineSyncTest)

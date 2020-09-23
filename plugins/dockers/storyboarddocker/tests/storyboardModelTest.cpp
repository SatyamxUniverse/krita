/*
 *  Copyright (c) 2020 Saurabh Kumar <saurabhk660@gmail.com>
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

#include "storyboardModelTest.h"

#include <QTest>
#include <QWidget>
#include <storyboardModel.h>
#include <commentModel.h>

void StoryboardModelTest::init()
{
    m_commentModel = new CommentModel(this);
    m_storyboardModel = new StoryboardModel(this);
    m_storyboardModel->setCommentModel(m_commentModel);

    m_commentModel->insertRows(m_commentModel->rowCount(),1);
    m_storyboardModel->insertRows(m_storyboardModel->rowCount(),1);
    QCOMPARE(m_commentModel->rowCount(), 1);
}

void StoryboardModelTest::cleanup()
{
    m_storyboardModel->removeRows(0, m_storyboardModel->rowCount());
    m_commentModel->removeRows(0, m_commentModel->rowCount());

    delete m_storyboardModel;
    delete m_commentModel;
}


void StoryboardModelTest::testAddComment()
{

    auto sbTester = new QAbstractItemModelTester(m_storyboardModel, this);
    auto commentTester = new QAbstractItemModelTester(m_commentModel, this);
    Q_UNUSED(sbTester);
    Q_UNUSED(commentTester);

    int rowStoryboard = m_storyboardModel->rowCount(m_storyboardModel->index(0,0));
    int rowsComment = m_commentModel->rowCount();

    QCOMPARE(rowStoryboard, rowsComment + 4);

    m_commentModel->insertRows(m_commentModel->rowCount(),1);

    QCOMPARE(rowsComment + 1, m_commentModel->rowCount());
    QCOMPARE(m_storyboardModel->rowCount(m_storyboardModel->index(0,0)), m_commentModel->rowCount() + 4);

    //add at an invalid position
    m_commentModel->insertRows(-1, 1);

    QCOMPARE(rowsComment + 1, m_commentModel->rowCount());
    QCOMPARE(m_storyboardModel->rowCount(m_storyboardModel->index(0,0)), m_commentModel->rowCount() + 4);
}

void StoryboardModelTest::testRemoveComment()
{

    auto sbTester = new QAbstractItemModelTester(m_storyboardModel, this);
    auto commentTester = new QAbstractItemModelTester(m_commentModel, this);
    Q_UNUSED(sbTester);
    Q_UNUSED(commentTester);

    int rowStoryboard = m_storyboardModel->rowCount(m_storyboardModel->index(0,0));
    int rowsComment = m_commentModel->rowCount();

    QCOMPARE(rowStoryboard, rowsComment + 4);

    m_commentModel->removeRows(m_commentModel->rowCount() - 1,1);

    QCOMPARE(rowsComment - 1, m_commentModel->rowCount());
    QCOMPARE(m_storyboardModel->rowCount(m_storyboardModel->index(0,0)), m_commentModel->rowCount() + 4);

    m_commentModel->removeRows(-1,1);

    QCOMPARE(rowsComment - 1, m_commentModel->rowCount());
    QCOMPARE(m_storyboardModel->rowCount(m_storyboardModel->index(0,0)), m_commentModel->rowCount() + 4);
}

void StoryboardModelTest::testCommentNameChanged()
{
    auto tester = new QAbstractItemModelTester(m_commentModel, this);
    Q_UNUSED(tester);
    QModelIndex index = m_commentModel->index(m_commentModel->rowCount() - 1, 0);
    QVariant value = QVariant(QString("newValue"));
    m_commentModel->setData(index, value);

    QCOMPARE(QString("newValue"), m_commentModel->data(index));
}

void StoryboardModelTest::testFrameAdded()
{
    int rows = m_storyboardModel->rowCount();
    auto tester = new QAbstractItemModelTester(m_storyboardModel, 0);
    Q_UNUSED(tester);
    m_storyboardModel->insertRows(m_storyboardModel->rowCount(),1);

    QCOMPARE(rows + 1, m_storyboardModel->rowCount());
}


void StoryboardModelTest::testFrameRemoved()
{
    int rows = m_storyboardModel->rowCount();
    auto tester = new QAbstractItemModelTester(m_storyboardModel, 0);
    Q_UNUSED(tester);
    m_storyboardModel->removeRows(m_storyboardModel->rowCount() - 1,1);

    QCOMPARE(rows-1, m_storyboardModel->rowCount());
}

void StoryboardModelTest::testFrameChanged()
{

    auto tester = new QAbstractItemModelTester(m_storyboardModel, 0);
    Q_UNUSED(tester);
    QModelIndex parentIndex = m_storyboardModel->index(m_storyboardModel->rowCount() - 1, 0);
    QModelIndex frameIndex = m_storyboardModel->index(0, 0, parentIndex);
    QVariant value = QVariant(100);
    m_storyboardModel->setData(frameIndex, value, Qt::EditRole);

    QCOMPARE(m_storyboardModel->data(frameIndex).toInt(), 100);

    //invalid value shouldn't change anything
    QVariant invalidValue = QVariant(-100);
    m_storyboardModel->setData(frameIndex, invalidValue, Qt::EditRole);

    QCOMPARE(m_storyboardModel->data(frameIndex).toInt(), 100);
}

void StoryboardModelTest::testDurationChanged()
{
    auto tester = new QAbstractItemModelTester(m_storyboardModel, 0);
    Q_UNUSED(tester);
    QModelIndex parentIndex = m_storyboardModel->index(m_storyboardModel->rowCount() - 1, 0);
    QModelIndex secIndex = m_storyboardModel->index(2, 0, parentIndex);
    QVariant value = QVariant(100);
    m_storyboardModel->setData(secIndex, value, Qt::EditRole);

    QCOMPARE(m_storyboardModel->data(secIndex).toInt(), 100);

    //invalid value shouldn't change anything
    QVariant invalidValue = QVariant(-100);
    m_storyboardModel->setData(secIndex, invalidValue, Qt::EditRole);

    QCOMPARE(m_storyboardModel->data(secIndex).toInt(), 100);
}

void StoryboardModelTest::testCommentChanged()
{
    auto tester = new QAbstractItemModelTester(m_storyboardModel, 0);
    Q_UNUSED(tester);
    QModelIndex parentIndex = m_storyboardModel->index(m_storyboardModel->rowCount() - 1, 0);
    QModelIndex commentIndex = m_storyboardModel->index(4, 0, parentIndex);
    QVariant value = QVariant(QString("newComment"));
    m_storyboardModel->setData(commentIndex, value, Qt::EditRole);

    QCOMPARE(m_storyboardModel->data(commentIndex).toString(), "newComment");
}

QTEST_MAIN(StoryboardModelTest)

/*
 *  Copyright (c) 2015 Jouni Pentikäinen <joupent@gmail.com>
 *  Copyright (c) 2020 Emmet O'Neill <emmetoneill.pdx@gmail.com>
 *  Copyright (c) 2020 Eoin O'Neill <eoinoneill1991@gmail.com>
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

#ifndef KIS_KEYFRAMING_TEST_H
#define KIS_KEYFRAMING_TEST_H

#include <QtTest>
#include "KoColor.h"


class KisKeyframingTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testChannelSignals();

    void testRasterChannel();
    void testRasterFrameFetching();
    void testRasterUndoRedo();
    void testFirstFrameOperations();
    void testInterChannelMovement();

    void testScalarChannel();
    void testScalarValueInterpolation();
    void testScalarChannelUndoRedo();
    void testScalarAffectedFrames();
    void testChangeOfScalarLimits();

private:
    const KoColorSpace *cs;

    quint8* red;
    quint8* green;
    quint8* blue;
};

#endif


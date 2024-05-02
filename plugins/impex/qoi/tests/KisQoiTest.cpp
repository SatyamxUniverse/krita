/*
 * SPDX-FileCopyrightText: 2019 Agata Cacko <cacko.azh@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisQoiTest.h"


#include <simpletest.h>
#include <QCoreApplication>

#include "filestest.h"

#ifndef FILES_DATA_DIR
#error "FILES_DATA_DIR not set. A directory with the data used for testing the importing of files in krita"
#endif


const QString QoiMimetype = "image/qoi";



void KisQoiTest::testImportFromWriteonly()
{
    TestUtil::testImportFromWriteonly(QoiMimetype);
}


void KisQoiTest::testExportToReadonly()
{
    TestUtil::testExportToReadonly(QoiMimetype);
}


void KisQoiTest::testImportIncorrectFormat()
{
    TestUtil::testImportIncorrectFormat(QoiMimetype);
}



KISTEST_MAIN(KisQoiTest)



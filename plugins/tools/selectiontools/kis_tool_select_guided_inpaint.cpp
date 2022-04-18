/*
 *  Copyright (c) 2017 Eugene Ingerman
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

/**
 * See kis_inpaint.cpp for inspiration. Right now, this file is almost empty as it is a TODO for adding 3 tones painting for guided selection.
 * Only directives have been left.
 */

#include <boost/multi_array.hpp>
#include <random>
#include <iostream>
#include <functional>


#include "kis_paint_device.h"
#include "kis_painter.h"
#include "kis_selection.h"

#include "kis_debug.h"
#include "kis_paint_device_debug_utils.h"
//#include "kis_random_accessor_ng.h"

#include <QList>
#include <kis_transform_worker.h>
#include <kis_filter_strategy.h>
#include "KoColor.h"
#include "KoColorSpace.h"
#include "KoChannelInfo.h"
#include "KoMixColorsOp.h"
#include "KoColorModelStandardIds.h"
#include "KoColorSpaceRegistry.h"
#include "KoColorSpaceTraits.h"


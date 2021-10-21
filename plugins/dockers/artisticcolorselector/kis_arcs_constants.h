/*
 *  SPDX-FileCopyrightText: 2018 Anna Medonosova <anna.medonosova@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_ARCS_CONSTANTS_H
#define KIS_ARCS_CONSTANTS_H

#include <QString>
#include <QColor>

static constexpr int MIN_NUM_HUE_PIECES = 1;
static constexpr int MIN_NUM_UI_HUE_PIECES = 2;
static constexpr int MAX_NUM_HUE_PIECES = 48;
static constexpr int MIN_NUM_LIGHT_PIECES = 1;
static constexpr int MIN_NUM_UI_LIGHT_PIECES = 2;
static constexpr int MAX_NUM_LIGHT_PIECES = 30;
static constexpr int MIN_NUM_SATURATION_RINGS = 1;
static constexpr int MAX_NUM_SATURATION_RINGS = 20;

static constexpr int DEFAULT_HUE_STEPS = 12;
static constexpr int DEFAULT_SATURATION_STEPS = 7;
static constexpr int DEFAULT_VALUE_SCALE_STEPS = 11;

static constexpr qreal DEFAULT_LUMA_R = 0.2126;
static constexpr qreal DEFAULT_LUMA_G = 0.7152;
static constexpr qreal DEFAULT_LUMA_B = 0.0722;
static constexpr qreal DEFAULT_LUMA_GAMMA = 2.2;

// color scheme for the selector
static constexpr QColor COLOR_MIDDLE_GRAY = QColor(128, 128, 128, 255);
static constexpr QColor COLOR_DARK = QColor(50, 50, 50, 255);
static constexpr QColor COLOR_LIGHT = QColor(200, 200, 200, 255);
static constexpr QColor COLOR_SELECTED_DARK = QColor(30, 30, 30, 255);
static constexpr QColor COLOR_SELECTED_LIGHT = QColor(220, 220, 220, 255);

static constexpr QColor COLOR_MASK_FILL = COLOR_MIDDLE_GRAY;
static constexpr QColor COLOR_MASK_OUTLINE = COLOR_LIGHT;
static constexpr QColor COLOR_MASK_CLEAR = COLOR_LIGHT;
static constexpr QColor COLOR_NORMAL_OUTLINE = COLOR_MIDDLE_GRAY;

#endif // KIS_ARCS_CONSTANTS_H

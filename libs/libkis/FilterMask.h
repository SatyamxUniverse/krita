/*
 *  Copyright (c) 2017 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef LIBKIS_FILTERMASK_H
#define LIBKIS_FILTERMASK_H

#include <QObject>
#include "Node.h"
#include "Filter.h"

#include <kis_types.h>

#include "kritalibkis_export.h"
#include "libkis.h"

/**
 * @brief The FilterMask class
 * A filter mask, unlike a filter layer, will add a non-destructive filter
 * to the composited image of the node it is attached to.
 *
 * You can set grayscale pixeldata on the filter mask to adjust where the filter is applied.
 *
 * Filtermasks can be animated.
 */

class KRITALIBKIS_EXPORT FilterMask : public Node
{
    Q_OBJECT
    Q_DISABLE_COPY(FilterMask)

public:
    explicit FilterMask(KisImageSP image, QString name, Filter &filter, QObject *parent = 0);
    explicit FilterMask(KisImageSP image, KisFilterMaskSP mask, QObject *parent=0);
    ~FilterMask() override;
public Q_SLOTS:

    /**
     * @brief type Krita has several types of nodes, split in layers and masks. Group
     * layers can contain other layers, any layer can contain masks.
     *
     * @return The type of the node. Valid types are:
     * <ul>
     *  <li>paintlayer
     *  <li>grouplayer
     *  <li>filelayer
     *  <li>filterlayer
     *  <li>filllayer
     *  <li>clonelayer
     *  <li>vectorlayer
     *  <li>transparencymask
     *  <li>filtermask
     *  <li>transformmask
     *  <li>selectionmask
     *  <li>colorizemask
     * </ul>
     *
     * If the Node object isn't wrapping a valid Krita layer or mask object, and
     * empty string is returned.
     */
    QString type() const override;

    void setFilter(Filter &filter);
    Filter *filter();
};

#endif // LIBKIS_FILTERMASK_H



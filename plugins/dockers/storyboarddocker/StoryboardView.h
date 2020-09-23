/*
  Copyright (c) 2020 Saurabh Kumar <saurabhk660@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef STORYBOARD_VIEW_H
#define STORYBOARD_VIEW_H

#include <QListView>
#include <QScroller>

#include <kritastoryboarddocker_export.h>

class QStyleOptionViewItem;
class StoryboardModel;

/**
 * This view draws the children of every index in the first column of 
 * the model inside the parent index
 *
 * */

class KRITASTORYBOARDDOCKER_EXPORT StoryboardView: public QListView
{
    Q_OBJECT
public:
    explicit StoryboardView(QWidget *parent = 0);
    ~StoryboardView() override;

    void paintEvent(QPaintEvent *event) override;
    QRect visualRect(const QModelIndex &index) const override;
    QModelIndex indexAt(const QPoint &point) const override;
    void setItemOrientation(Qt::Orientation orientation);

    /**
     * @brief whether Comments are below or on the right of Thumbnail
     * @return The orientation of each Storyboard Item
     */
    Qt::Orientation itemOrientation();

    /**
     * @return True if comments are visible, otherwise False.
     */
    bool commentIsVisible() const;

    /**
     * @return True if thumbnails are visible, otherwise False.
     */
    bool thumbnailIsVisible() const;

    /**
     * @brief Sets the visibility of comments
     * @param value The new visiblity value
     */
    void setCommentVisibility(bool value);

    /**
     * @brief Sets the visibility of thumbnails
     * @param value The new visiblity value
     */
    void setThumbnailVisibility(bool value);

    /**
     * @brief changes the @c currentIndex and @c selectedIndex to frame
     * @param frame The new current frame
     */
    void setCurrentItem(int frame);

private Q_SLOTS:
    void slotContextMenuRequested(const QPoint &);

private:
    Qt::Orientation m_itemOrientation;
    bool m_commentIsVisible;
    bool m_thumbnailIsVisible;
};

#endif

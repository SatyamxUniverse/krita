/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISPANACTIONWATCHER_H
#define KISPANACTIONWATCHER_H

#include <QObject>

#include <kritaui_export.h>

class KisCanvas2;

/**
 * @brief This class tracks the input events looking for the start/end of a
 *        pan action, and also can trigger the action itself.
 * 
 *        Usually, the pan action is triggered automatically by the input
 *        manager, unless the current tool is being used (the user is "painting",
 *        or, in other words, the tool activation key/button, usually the mouse
 *        left button, is pressed). So this class is useful when a tool wants
 *        to be able to activate the pan action even when the user is "painting".
 * 
 *        An example is panning while making a freehand selection at some large
 *        zoom level to gain precision. In general it can be useful when a tool
 *        action requires a potentially long drag through the canvas.
 * 
 *        The shortcuts to activate the pan action are the same as the general
 *        pan canvas input shortcuts set in the current profile. Those are
 *        respected even when the user changes the profile.
 * 
 *        WARNING: Use this class only if panning while "painting" is needed. If
 *        the user is not "painting" then the input manager will trigger the
 *        action when some relevant shortcut is activated. Generally the action
 *        activation should be enabled in the tool in some method like
 *        "beginPrimaryAction" (when the user activates the tool primary action),
 *        and disabled in "endPrimaryAction".
 * 
 */
class KRITAUI_EXPORT KisPanActionWatcher : public QObject
{
    Q_OBJECT

public:
    KisPanActionWatcher();
    ~KisPanActionWatcher() override;

    /**
     * @brief Activate the pan action watcher. This method will connect to the
     *        input manager and start tracking the input events
     * @param canvas The canvas object used to retrieve the input manager
     */
    void activate(KisCanvas2 *canvas);
    /**
     * @brief Deactivate the pan action watcher. This method will disconnect
     *        from the input manager and stop tracking the input events
     */
    void deactivate();
    /**
     * @brief Tell if the pan action watcher is currently tracking input events
     */
    bool isActive() const;

    /**
     * @brief If @param enabled is true then the pan action will be triggered
     *        is some relevant shortcut is pressed. If it is false, then nothing
     *        will happen and the current pan action, if any, will be terminated
     */
    void setPanActionEnabled(bool enabled);
    /**
     * @brief Tell if the pan action can be triggered when some relevant
     *        shortcut is pressed
     */
    bool isPanActionEnabled() const;
    /**
     * @brief Tell if some relevant shortcut was pressed and a pan action is
     *        currently active
     */
    bool isPanning() const;

private:
    class Private;
    QScopedPointer<Private> m_d;

    bool eventFilter(QObject *o, QEvent *e) override;
};

#endif

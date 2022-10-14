/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>

#include <kis_canvas2.h>
#include <input/kis_input_manager.h>
#include <input/kis_input_profile_manager.h>
#include <input/kis_input_profile.h>
#include <input/kis_shortcut_configuration.h>
#include <input/kis_abstract_input_action.h>
#include <input/kis_pan_action.h>
#include <kis_assert.h>

#include "KisPanActionWatcher.h"

class KisPanActionWatcher::Private
{
public:
    Private(KisPanActionWatcher *q)
        : m_q(q)
    {}
    
    void startListeningForInputEvents()
    {
        KisInputManager *inputManager = m_canvas->globalInputManager();
        if (inputManager) {
            inputManager->attachLowLevelPriorityEventFilter(m_q);
        }
    }

    void stopListeningForInputEvents()
    {
        KisInputManager *inputManager = m_canvas->globalInputManager();
        if (inputManager) {
            inputManager->detachLowLevelPriorityEventFilter(m_q);
        }
    }

    void startListeningForInputProfileChanges()
    {
        m_inputProfileConnectionHandle =
            QObject::connect(
                KisInputProfileManager::instance(),
                &KisInputProfileManager::currentProfileChanged,
                [this](){
                    reloadShortcuts();
                }
            );
    }

    void stopListeningForInputProfileChanges()
    {
       QObject::disconnect(m_inputProfileConnectionHandle);
    }

    void clearShortcuts()
    {
        m_shortcuts.clear();
        m_activeShortcut = nullptr;
        m_trackedKeys.clear();
        m_trackedMouseButtons = Qt::NoButton;
    }

    void reloadShortcuts()
    {
        clearShortcuts();

        KisInputProfile *currentInputProfile =
            KisInputProfileManager::instance()->currentProfile();
        if (currentInputProfile) {
            for (KisShortcutConfiguration *shortcut :
                    currentInputProfile->shortcutsForAction("Pan Canvas")) {
                // Only use [key] and [key + mouse_button] shortcuts
                if (shortcut &&
                    (shortcut->type() == KisShortcutConfiguration::KeyCombinationType ||
                     shortcut->type() == KisShortcutConfiguration::MouseButtonType)) {
                    const QList<Qt::Key> keys = shortcut->keys();
                    if (shortcut->type() == KisShortcutConfiguration::KeyCombinationType) {
                        // Discard shortcuts that have no keys assigned
                        if (keys.isEmpty()) {
                            continue;
                        }
                    } else {
                        const Qt::MouseButtons mouseButtons = shortcut->buttons();
                        // Discard shortcuts that have no mouse buttons assigned
                        if (!mouseButtons) {
                            continue;
                        }
                        m_trackedMouseButtons |= mouseButtons;
                    }
                    for (Qt::Key key : keys) {
                        m_trackedKeys << key;
                    }
                    m_shortcuts << shortcut;
                }
            }
        }
    }

    void activate(KisCanvas2 *canvas)
    {
        KIS_SAFE_ASSERT_RECOVER_RETURN(canvas);
        KIS_SAFE_ASSERT_RECOVER_RETURN(canvas != m_canvas);

        if (isActive()) {
            deactivate();
        }
        m_canvas = canvas;
        reloadShortcuts();
        startListeningForInputProfileChanges();
        startListeningForInputEvents();
    }

    void deactivate()
    {
        KIS_SAFE_ASSERT_RECOVER_RETURN(isActive());

        stopListeningForInputEvents();
        stopListeningForInputProfileChanges();
        clearShortcuts();
        m_canvas = nullptr;
    }

    bool isActive() const
    {
        return m_canvas;
    }

    bool isPanActionEnabled() const
    {
        return m_isPanActionEnabled;
    }

    void setPanActionEnabled(bool enabled)
    {
        if (enabled == m_isPanActionEnabled) {
            return;
        }
        m_isPanActionEnabled = enabled;
        if (isShortcutActive()) {
            if (isPanActionEnabled()) {
                activatePanAction();
                beginPanAction();
            } else {
                endPanAction();
                deactivatePanAction();
            }
        }
    }

    bool isShortcutActive() const
    {
        return m_activeShortcut;
    }

    bool isPanning() const
    {
        return isPanActionEnabled() && isShortcutActive();
    }

    void activatePanAction()
    {
        KIS_ASSERT(isShortcutActive());

        KisAbstractInputAction *action = m_activeShortcut->action();
        const int mode = m_activeShortcut->mode();
        action->activate(mode);
    }

    void deactivatePanAction()
    {
        KIS_ASSERT(isShortcutActive());

        KisAbstractInputAction *action = m_activeShortcut->action();
        const int mode = m_activeShortcut->mode();
        action->deactivate(mode);
    }

    void beginPanAction()
    {
        KIS_ASSERT(isShortcutActive());

        KisAbstractInputAction *action = m_activeShortcut->action();
        const int mode = m_activeShortcut->mode();
        QMouseEvent me(QEvent::MouseButtonPress, m_lastMousePos, Qt::LeftButton,
                       Qt::LeftButton, Qt::NoModifier);
        action->begin(mode, &me);
    }

    void continuePanAction()
    {
        KIS_ASSERT(isShortcutActive());

        KisAbstractInputAction *action = m_activeShortcut->action();
        if (m_activeShortcut->mode() == KisPanAction::PanModeShortcut) {
            QMouseEvent me(QEvent::MouseMove, m_lastMousePos, Qt::LeftButton,
                           Qt::LeftButton, Qt::NoModifier);
            action->inputEvent(&me);
        }
    }

    void endPanAction()
    {
        KIS_ASSERT(isShortcutActive());

        KisAbstractInputAction *action = m_activeShortcut->action();
        action->end(nullptr);
    }

    bool inputMatchesShortcut(KisShortcutConfiguration *shortcut) const
    {
        KIS_ASSERT(shortcut);

        for (Qt::Key key : shortcut->keys()) {
            if (!m_pressedKeys.contains(key)) {
                return false;
            }
        }
        if (shortcut->type() == KisShortcutConfiguration::MouseButtonType) {
            auto buttonMatches =
                [shortcut, this](Qt::MouseButton button) {
                    const bool requiresButton = shortcut->buttons().testFlag(button);
                    if (!requiresButton) {
                        return true;
                    }
                    return m_pressedMouseButtons.testFlag(button) ||
                        m_pressedTabletButtons.testFlag(button);
                };
            if (!buttonMatches(Qt::LeftButton) ||
                !buttonMatches(Qt::MiddleButton) ||
                !buttonMatches(Qt::RightButton)) {
                return false;
            }
        }
        return true;
    }

    void resolveActiveShortcut()
    {
        // See if the current shortcut still matches the input. In that case
        // we keep it. We finish the current action, if any, otherwise
        if (isShortcutActive()) {
            if (inputMatchesShortcut(m_activeShortcut)) {
                return;
            } else {
                if (isPanning()) {
                    endPanAction();
                    deactivatePanAction();
                }
                m_activeShortcut = nullptr;
            }
        }
        // See if any of the shortcuts matches the current input
        for (KisShortcutConfiguration *shortcut : m_shortcuts) {
            if (inputMatchesShortcut(shortcut)) {
                m_activeShortcut = shortcut;
                if (isPanActionEnabled()) {
                    activatePanAction();
                    beginPanAction();
                }
                break;
            }
        }
    }

    bool eventFilter(QObject *o, QEvent *e)
    {
        Q_UNUSED(o);

        if (e->type() == QEvent::ShortcutOverride) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->isAutoRepeat()) {
                // We only use the auto-repeated keys if a pan action
                // activated by key presses is active, and the shortcut used to
                // activate it uses the current key
                if (isPanning() &&
                    m_activeShortcut->type() == KisShortcutConfiguration::KeyCombinationType &&
                    m_activeShortcut->mode() != KisPanAction::PanModeShortcut &&
                    m_activeShortcut->keys().contains(static_cast<Qt::Key>(ke->key()))) {
                    beginPanAction();
                }
            } else {
                if (m_trackedKeys.contains(static_cast<Qt::Key>(ke->key()))) {
                    m_pressedKeys.insert(static_cast<Qt::Key>(ke->key()));
                    resolveActiveShortcut();
                }
            }

        } else if (e->type() == QEvent::KeyRelease) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->isAutoRepeat()) {
                // See comment on auto-repeated keys for QEvent::ShortcutOverride
                if (isPanning() &&
                    m_activeShortcut->type() == KisShortcutConfiguration::KeyCombinationType &&
                    m_activeShortcut->mode() != KisPanAction::PanModeShortcut &&
                    m_activeShortcut->keys().contains(static_cast<Qt::Key>(ke->key()))) {
                    endPanAction();
                }
            } else {
                if (m_pressedKeys.contains(static_cast<Qt::Key>(ke->key()))) {
                    m_pressedKeys.remove(static_cast<Qt::Key>(ke->key()));
                    resolveActiveShortcut();
                }
            }

        } else if (e->type() == QEvent::MouseButtonPress ||
                   e->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            if (m_trackedMouseButtons.testFlag(me->button())) {
                m_pressedMouseButtons.setFlag(me->button(), true);
                resolveActiveShortcut();
            }

        } else if (e->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            m_pressedMouseButtons.setFlag(me->button(), false);
            resolveActiveShortcut();

        } else if (e->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            m_lastMousePos = me->localPos();
            if (isPanning()) {
                continuePanAction();
            }

        } else if (e->type() == QEvent::TabletPress) {
            QTabletEvent *te = static_cast<QTabletEvent*>(e);
            if (m_trackedMouseButtons.testFlag(te->button())) {
                m_pressedTabletButtons.setFlag(te->button(), true);
                resolveActiveShortcut();
            }

        } else if (e->type() == QEvent::TabletRelease) {
            QTabletEvent *te = static_cast<QTabletEvent*>(e);
            if (m_trackedMouseButtons.testFlag(te->button())) {
                m_pressedTabletButtons.setFlag(te->button(), false);
                resolveActiveShortcut();
            }
            
        } else if (e->type() == QEvent::TabletMove) {
            QTabletEvent *te = static_cast<QTabletEvent*>(e);
            m_lastMousePos = te->posF();
            if (isPanning()) {
                continuePanAction();
            }
        }

        return false;
    }

private:
    KisPanActionWatcher *m_q {nullptr};
    KisCanvas2 *m_canvas {nullptr};
    bool m_isPanActionEnabled {false};
    QList<KisShortcutConfiguration*> m_shortcuts;
    KisShortcutConfiguration *m_activeShortcut {nullptr};
    QSet<Qt::Key> m_trackedKeys;
    Qt::MouseButtons m_trackedMouseButtons {Qt::NoButton};
    Qt::MouseButtons m_trackedTabletButtons {Qt::NoButton};
    QSet<Qt::Key> m_pressedKeys;
    // Track mouse and tablet buttons separately in case these inputs are mixed
    // for some reason
    Qt::MouseButtons m_pressedMouseButtons {Qt::NoButton};
    Qt::MouseButtons m_pressedTabletButtons {Qt::NoButton};
    QPointF m_lastMousePos;
    QMetaObject::Connection m_inputProfileConnectionHandle;
};

KisPanActionWatcher::KisPanActionWatcher()
    : m_d(new Private(this))
{}

KisPanActionWatcher::~KisPanActionWatcher()
{}

void KisPanActionWatcher::activate(KisCanvas2 *canvas)
{
    m_d->activate(canvas);
}

void KisPanActionWatcher::deactivate()
{
    m_d->deactivate();
}

bool KisPanActionWatcher::isActive() const
{
    return m_d->isActive();
}

bool KisPanActionWatcher::isPanActionEnabled() const
{
    return m_d->isPanActionEnabled();
}

void KisPanActionWatcher::setPanActionEnabled(bool enabled)
{
    m_d->setPanActionEnabled(enabled);
}

bool KisPanActionWatcher::eventFilter(QObject *o, QEvent *e)
{
    return m_d->eventFilter(o, e);
}

bool KisPanActionWatcher::isPanning() const
{
    return m_d->isPanning();
}

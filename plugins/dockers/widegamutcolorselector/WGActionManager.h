/*
 * SPDX-FileCopyrightText: 2021 Mathias Wein <lynx.mw+kde@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WGPOPUPMANAGER_H
#define WGPOPUPMANAGER_H

#include <KisVisualColorModel.h>
#include <QObject>

class QAction;
class KisCanvas2;
class KisSignalCompressor;

class WGColorSelectorDock;
class WGSelectorPopup;

class WGActionManager : public QObject
{
    Q_OBJECT
public:
    explicit WGActionManager(WGColorSelectorDock *parentDock = nullptr);

    void setCanvas(KisCanvas2* canvas, KisCanvas2* oldCanvas);
private Q_SLOTS:
    void slotShowColorSelectorPopup();
    void slotChannelValuesChanged();
    void slotUpdateDocker();
Q_SIGNALS:
private:
    WGColorSelectorDock *m_docker {0};
    KisSignalCompressor *m_colorChangeCompressor;
    QAction *m_colorSelectorPopupAction {0};
    QAction *m_shadeSelectorPopupAction;
    QAction *m_colorHistoryPopupAction;
    WGSelectorPopup *m_colorSelectorPopup {0};
    KisVisualColorModelSP m_colorModel;
    bool m_isSynchronizing {false};
};

#endif // WGPOPUPMANAGER_H
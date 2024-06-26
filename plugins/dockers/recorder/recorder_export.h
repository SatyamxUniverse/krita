/*
 *  SPDX-FileCopyrightText: 2020 Dmitrii Utkin <loentar@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.1-only
 */

#ifndef RECORDER_EXPORT_H
#define RECORDER_EXPORT_H

#include "recorder_format.h"

#include <QDialog>

struct RecorderExportSettings;

class RecorderExport : public QDialog
{
    Q_OBJECT

public:
    explicit RecorderExport(RecorderExportSettings *s, QWidget *parent = nullptr);
    ~RecorderExport();

    void setup();

protected:
    void closeEvent(QCloseEvent *event) override;

private Q_SLOTS:
    void reject() override;

    // first page
    void onButtonBrowseDirectoryClicked();
    void onSpinInputFpsValueChanged(int value);
    void onSpinFpsValueChanged(int value);
    void onCheckResultPreviewToggled(bool checked);
    void onFirstFrameSecValueChanged(int value);
    void onCheckExtendResultToggled(bool checked);
    void onLastFrameSecValueChanged(int value);
    void onCheckResizeToggled(bool checked);
    void onSpinScaleWidthValueChanged(int value);
    void onSpinScaleHeightValueChanged(int value);
    void onButtonLockRatioToggled(bool checked);
    void onButtonLockFpsToggled(bool checked);
    void onButtonBrowseFfmpegClicked();
    void onComboProfileIndexChanged(int index);
    void onButtonEditProfileClicked();
    void onEditVideoPathChanged(const QString &videoFilePath);
    void onButtonBrowseExportClicked();
    void onButtonExportClicked();
    // second page
    void onButtonCancelClicked();
    // ffmpeg
    void onFFMpegStarted();
    void onFFMpegFinished();
    void onFFMpegFinishedWithError(QString error);
    void onFFMpegProgressUpdated(int frameNo);
    // third page
    void onButtonWatchItClicked();
    void onButtonShowInFolderClicked();
    void onButtonRemoveSnapshotsClicked();
    void onButtonRestartClicked();
    void onCleanUpFinished();

private:
     bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Q_DISABLE_COPY(RecorderExport)
    class Private;
    RecorderExportSettings *settings;
    QScopedPointer<Private> d;
};

#endif // RECORDER_EXPORT_H

#pragma once

#include <QAudioDeviceInfo>
#include <QWidget>
#include <QCamera>
#include <memory>
class QCameraImageCapture;

namespace Ui {
class MainView;
}

class AudioCapture;
QT_END_NAMESPACE

class MainView : public QWidget {
    Q_OBJECT

public:
    explicit MainView(QWidget *parent = nullptr);
    ~MainView() override;
    void setCamera(QCameraInfo camerainfo,
                   QAbstractVideoSurface *surface = nullptr);
    void setAudio(QAudioDeviceInfo audioinfo);
Q_SIGNALS:
    void stateChanged(QCamera::State state);

public Q_SLOTS:
    void startSlot(bool isChecked) const;
    void settingsSlot()const;
    void startPush(QString url );
public:
    std::unique_ptr<QCamera> mCamera{};
    std::unique_ptr<Ui::MainView> ui{};
    std::unique_ptr<AudioCapture> mAudioCapture{};
    std::unique_ptr<QCameraImageCapture> mImageCapture{};
};

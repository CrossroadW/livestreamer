#pragma once

#include <QAudioDeviceInfo>
#include <QWidget>
#include <QCamera>
#include <memory>
#include <QVideoFrame>
class QCameraImageCapture;
class AacEncoder;
class CBaseCameraSurface;
namespace Ui {
class MainView;
}

class VideoEncodeFF;
class AudioCapture;
namespace live{class RtmpPushLibrtmp;}
QT_END_NAMESPACE

class MainView : public QWidget {
    Q_OBJECT

public:
    explicit MainView(QWidget *parent = nullptr);
    ~MainView() override;
    void setCamera(const QCameraInfo& camerainfo);
    void setAudio(QAudioDeviceInfo audioinfo);
Q_SIGNALS:
    void stateChanged(QCamera::State state);

public Q_SLOTS:

    void settingsSlot() const;
    void startPush(const QString& url);
    void stopPush();
    void recvVFrame(QVideoFrame& frame);
    void recvAFrame(const char* data, qint64 len);
public:
    std::unique_ptr<QCamera> mCamera{};
    std::unique_ptr<Ui::MainView> ui{};
    std::unique_ptr<AudioCapture> mAudioCapture{};
    std::unique_ptr<QCameraImageCapture> mImageCapture{};
    std::unique_ptr<VideoEncodeFF> mVideoEncoder{};
    std::unique_ptr<AacEncoder> maacEncoder;
    std::unique_ptr<live::RtmpPushLibrtmp> rtmpPush;
    std::unique_ptr<CBaseCameraSurface> mSurface;
    struct Impl;
    std::unique_ptr<Impl> d;
};

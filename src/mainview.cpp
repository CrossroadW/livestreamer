#include "mainview.h"
#include <spdlog/spdlog.h>
#include <QCamera>
#include <QCameraInfo>
#include <QAbstractVideoSurface>

#include <QAudioDeviceInfo>
#include "../ui/ui_mainview.h"
#include "audiocapture.h"
#include <QCamera>
#include <QCameraImageCapture>
#include "imagesettings.h"
using namespace std;
using namespace spdlog;
#define PREFIX "[MainView] "

MainView::MainView(QWidget *parent) :
    QWidget(parent), ui(new Ui::MainView) {
    trace(PREFIX "Constructor called");
    ui->setupUi(this);
}

MainView::~MainView() {
    trace(PREFIX "Destructor called");
}

void MainView::setCamera(QCameraInfo cameraInfo,
                         QAbstractVideoSurface *surface) {
    if (mCamera && cameraInfo == QCameraInfo(*mCamera)) {
        trace(PREFIX "Camera is already set");
        return;
    }

    trace(PREFIX "Previous camera deleted");

    if (cameraInfo.isNull()) {
        trace(PREFIX "Invalid cameraInfo provided");
        return;
    }
    QCamera::State old_state{QCamera::UnloadedState};
    old_state = mCamera ? mCamera->state() : old_state;
    mCamera = std::make_unique<QCamera>(cameraInfo);
    connect(mCamera.get(), &QCamera::stateChanged, this,
            &MainView::stateChanged);
    switch (old_state) {
    case QCamera::ActiveState: mCamera->start();
        break;
    case QCamera::LoadedState: mCamera->stop();
        break;
    default: break;
    }
    mImageCapture = std::make_unique<QCameraImageCapture>(mCamera.get());
    connect(mImageCapture.get(),
            &QCameraImageCapture::readyForCaptureChanged,
            this, [] {});
    connect(mImageCapture.get(), &QCameraImageCapture::imageCaptured,
            this, [](int id, const QImage &preview) {
                info("imageCaptured: {}", id);
            });
    connect(mImageCapture.get(), &QCameraImageCapture::imageSaved,
            this, [](int id, const QString &fileName) {
                spdlog::info("imageSaved: {}, {}", id, fileName.toStdString());
            });

    if (surface) {
        mCamera->setViewfinder(surface);
        trace(PREFIX "Camera viewfinder set to custom surface");
    } else {
        mCamera->setViewfinder(ui->finderview);
        trace(PREFIX "Camera viewfinder set to ui->finderview");
    }
}

void MainView::setAudio(QAudioDeviceInfo audioInfo) {
    if (mAudioCapture) {
        trace(PREFIX "Previous audio capture deleted");
    }
    if (audioInfo.isNull()) {
        trace(PREFIX "Invalid audioInfo provided");
        return;
    }
    auto running = mAudioCapture ? mAudioCapture->isRunning() : false;
    auto tmp = make_unique<AudioCapture>();
    bool ok = tmp->Init(audioInfo);
    if (!ok) {
        warn(PREFIX "Audio capture init failed");
        return;
    }
    mAudioCapture = std::move(tmp);
    connect(mAudioCapture.get(), &AudioCapture::updateLevel, this,
            [this]() {
                ui->progress->setLevel(mAudioCapture->level());
            });
    if (running) {
        mAudioCapture->Start();
    }
}

void MainView::startSlot(bool isChecked) const {
    if (mCamera) {
        auto state = mCamera->state();
        if (isChecked) {
            if (state != QCamera::ActiveState) {
                mCamera->start();
                trace(PREFIX "Camera started");
            } else {
                trace(PREFIX "Camera already started");
            }
        } else {
            if (state != QCamera::LoadedState) {
                mCamera->stop();
                trace(PREFIX "Camera stopped");
            } else {
                trace(PREFIX "Camera already stopped");
            }
        }
    } else {
        warn(PREFIX "Camera is nullptr");
    }

    // 处理麦克风
    if (mAudioCapture) {
        if (isChecked) {
            if (!mAudioCapture->isRunning()) {
                mAudioCapture->Start();
                trace(PREFIX "Microphone started");
            } else {
                trace(PREFIX "Microphone already started");
            }
        } else {
            if (mAudioCapture->isRunning()) {
                mAudioCapture->Stop();
                trace(PREFIX "Microphone stopped");
            } else {
                trace(PREFIX "Microphone already stopped");
            }
        }
    } else {
        warn(PREFIX "AudioCapture is nullptr");
    }
}

void MainView::settingsSlot() const {
    trace(PREFIX "Settings button clicked");
    ImageSettings settingsDialog(mImageCapture.get());

    QImageEncoderSettings settings = mImageCapture->encodingSettings();
    settingsDialog.setImageSettings(settings);
    if (settingsDialog.exec()) {
        spdlog::info("configureImageSettings");
        settings = settingsDialog.imageSettings();
        mImageCapture->setEncodingSettings(settings);
    }
}

void MainView::startPush(QString url) {
    info(PREFIX "startPush: {}", url.toStdString());
}

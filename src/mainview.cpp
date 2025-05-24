#include "mainview.h"
#include <spdlog/spdlog.h>
#include <QCamera>
#include <QCameraInfo>
#include "aac_encoder.h"
#include <QAudioDeviceInfo>
#include "../ui/ui_mainview.h"
#include "audiocapture.h"
#include <QCameraImageCapture>
#include "imagesettings.h"
#include <array>
#include <memory>
#include "video_encodeff.h"
#include "RtmpPushLibrtmp.h"
#include "CBaseCameraSurface.h"
using namespace std;
using namespace spdlog;
#define PREFIX "[MainView] "
#define RGB2Y(r,g,b) \
((unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8)+16)

#define RGB2U(r,g,b) \
((unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128)

#define RGB2V(r,g,b) \
((unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128)

struct MainView::Impl {
    std::vector<uint8_t> audio_encoder_data = std::vector<
        uint8_t>(1024 * 2 * 2);
    std::vector<uint8_t> video_encoder_data = std::vector<uint8_t>(1024 * 1024);
    int width_{};
    int height_{};
    unsigned char *dst_yuv_420{};
    FILE *rgb_out_file{};

    unsigned char clip_value(unsigned char x, unsigned char min_val,
                             unsigned char max_val) {
        if (x > max_val) {
            return max_val; //大于max_val的值记作max_val
        } else if (x < min_val) {
            return min_val; //小于min_val的值记作min_val
        } else {
            return x; //在指定范围内，就保持不变
        }
    }
};


MainView::MainView(QWidget *parent) :
    QWidget(parent), ui(new Ui::MainView), d{make_unique<Impl>()} {
    trace(PREFIX "Constructor called");
    ui->setupUi(this);
}

MainView::~MainView() {
    trace(PREFIX "Destructor called");
}

void MainView::setCamera(const QCameraInfo& cameraInfo) {
    trace(PREFIX "Previous camera deleted");
    mCamera = std::make_unique<QCamera>(cameraInfo);
    mImageCapture = std::make_unique<QCameraImageCapture>(mCamera.get());
    connect(mImageCapture.get(),
            &QCameraImageCapture::readyForCaptureChanged,
            this, [] {});
    connect(mImageCapture.get(), &QCameraImageCapture::imageCaptured,
            this, [](int id, const QImage &) {
                info("imageCaptured: {}", id);
            });
    connect(mImageCapture.get(), &QCameraImageCapture::imageSaved,
            this, [](int id, const QString &fileName) {
                info("imageSaved: {}, {}", id, fileName.toStdString());
            });

    mSurface = make_unique<CBaseCameraSurface>();
    mCamera->setViewfinder(mSurface.get());
    connect(mSurface.get(), &CBaseCameraSurface::frameAvailable,
            this, &MainView::recvVFrame);
    ui->stackedWidget->setCurrentIndex(1);

    mCamera->start();
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
    mAudioCapture->Start();
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

void MainView::startPush(const QString &url) {
    using namespace live;

    ui->stackedWidget->setCurrentIndex(1);
    info(PREFIX "Camera viewfinder set to custom surface");
    info(PREFIX "startPush: {}", url.toStdString());
    mAudioCapture->OpenWrite();
    maacEncoder = make_unique<AacEncoder>();

    int sample_rate = mAudioCapture->format().sample_rate;
    int channel_layout = mAudioCapture->format().chanel_layout;
    AVSampleFormat smaple_fmt = mAudioCapture->format().sample_fmt;
    maacEncoder->InitEncode(sample_rate, 96000, smaple_fmt, channel_layout);

    mAudioCapture->InitDecDataSize(maacEncoder->frame_byte_size);

    if (!mVideoEncoder) {
        mVideoEncoder = make_unique<VideoEncodeFF>();
        mVideoEncoder->InitEncode(d->width_, d->height_, 25, 2000000, "main");
    }
    rtmpPush = std::make_unique<RtmpPushLibrtmp>();
    VideoEncoder::VCodecConfig &vcode_info = mVideoEncoder->GetCodecConfig();
    AudioEncoder::ACodecConfig &acode_info = maacEncoder->GetCodecConfig();
    rtmpPush->Init(url.toStdString(),
                   vcode_info.width, vcode_info.height, vcode_info.fps,
                   acode_info.sample_rate, acode_info.channel);

    rtmpPush->SendAacSpec(maacEncoder->GetExterdata(),
                          maacEncoder->GetExterdataSize());
    rtmpPush->SendSpsPps(mVideoEncoder->GetExterdata(),
                         mVideoEncoder->GetExterdataSize());
}

void MainView::stopPush() {
    info(PREFIX "stopPush");


    mAudioCapture->CloseWrite();
    ui->stackedWidget->setCurrentIndex(0);

    if (maacEncoder) {
        maacEncoder->StopEncode();
        maacEncoder.reset(nullptr);
    }

    if (mVideoEncoder) {
        mVideoEncoder->StopEncode();
        mVideoEncoder.reset(nullptr);
    }

    d->audio_encoder_data.clear();
    d->video_encoder_data.clear();

    if (rtmpPush) {
        rtmpPush->Close();
    }
}

void MainView::recvVFrame(QVideoFrame &frame) {
    frame.map(QAbstractVideoBuffer::ReadOnly);
    // 获取 YUV 编码的原始数据
    QVideoFrame::PixelFormat pixelFormat = frame.pixelFormat();

    d->width_ = frame.width();
    d->height_ = frame.height();

    //需要写文件的时候才去转换数据
    if (mVideoEncoder) {
        if (pixelFormat == QVideoFrame::Format_RGB32) {
            int width = frame.width();
            int height = frame.height();

            if (d->dst_yuv_420 == nullptr) {
                d->dst_yuv_420 = new uchar[width * height * 3 / 2];
                memset(d->dst_yuv_420, 0, width * height * 3 / 2);
            }

            int planeCount = frame.planeCount();
            int mapBytes = frame.mappedBytes();
            int rgb32BytesPerLine = frame.bytesPerLine();
            const uchar *data = frame.bits();
            if (data == nullptr) {
                return;
            }

            QVideoFrame::FieldType fileType = frame.fieldType();

            int idx = 0;
            int idxu = 0;
            int idxv = 0;
            for (int i = height - 1; i >= 0; i--) {
                for (int j = 0; j < width; j++) {
                    uchar b = data[(width * i + j) * 4];
                    uchar g = data[(width * i + j) * 4 + 1];
                    uchar r = data[(width * i + j) * 4 + 2];
                    uchar a = data[(width * i + j) * 4 + 3];

                    if (d->rgb_out_file) {
                        //fwrite(&b, 1, 1, rgb_out_file);
                        //fwrite(&g, 1, 1, rgb_out_file);
                        //fwrite(&r, 1, 1, rgb_out_file);
                        //fwrite(&a, 1, 1, rgb_out_file);
                    }

                    uchar y = RGB2Y(r, g, b);
                    uchar u = RGB2U(r, g, b);
                    uchar v = RGB2V(r, g, b);

                    d->dst_yuv_420[idx++] = d->clip_value(y, 0, 255);
                    if (j % 2 == 0 && i % 2 == 0) {
                        d->dst_yuv_420[width * height + idxu++] = d->clip_value(
                            u, 0, 255);
                        d->dst_yuv_420[width * height * 5 / 4 + idxv++] =
                            d->clip_value(v, 0, 255);
                    }
                }
            }

            int len = mVideoEncoder->Encode(d->dst_yuv_420,
                                            d->video_encoder_data.data());
            if (rtmpPush && len > 0) {
                rtmpPush->SendVideo(d->video_encoder_data.data(), len);
            }
        }

        QImage::Format qImageFormat = QVideoFrame::imageFormatFromPixelFormat(
            frame.pixelFormat());
        QImage videoImg(frame.bits(),
                        frame.width(),
                        frame.height(),
                        qImageFormat);

        videoImg = videoImg.convertToFormat(qImageFormat).
                            mirrored(false, false);

        info(PREFIX "recvVFrame: {}x{}", videoImg.width(),
             videoImg.height());
        ui->customlabel->setPixmap(QPixmap::fromImage(videoImg));
    }
}

void MainView::recvAFrame(const char *data, qint64 len) {
    if (maacEncoder) {
        int ret_len = maacEncoder->Encode(data, len,
                                          d->audio_encoder_data.data());

        if (rtmpPush && ret_len > 0) {
            rtmpPush->SendAudio(d->audio_encoder_data.data(), ret_len);
        }
    }
}

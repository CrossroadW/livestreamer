#include <QInputDialog>
#include "mainwindow.h"
#include <QDir>
#include <QCameraInfo>
#include <QCamera>
#include <QMenu>
#include <QMainWindow>
#include <spdlog/spdlog.h>
#include <QWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QToolButton>
#include <QMetaType>
#include "mainview.h"
#include <cassert>
#include <QCameraImageCapture>
#define PREFIX  "[MainWindow] "
#include "CBaseCameraSurface.h"
#include "../ui/ui_mainview.h"
using namespace std;
using namespace spdlog;

Q_DECLARE_METATYPE(QCameraInfo)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), m_view{new MainView()} {
    setCentralWidget(m_view);
    resize(1280, 720);

    m_toolbar = addToolBar("topToolBar");

    connect(m_view, &MainView::stateChanged, this,
            [this](const QCamera::State state) {
                switch (state) {
                case QCamera::ActiveState: statusBar()->showMessage("正在录制");

                    break;
                case QCamera::UnloadedState: statusBar()->showMessage("未加载相机");
                    break;
                default: statusBar()->showMessage("已加载");
                }
            });
    statusBar()->showMessage("准备");

    {
        m_toolButton = new QToolButton();
        m_toolbar->addWidget(m_toolButton);
        m_toolButton->setText("相机选择");
        m_toolButton->setPopupMode(QToolButton::InstantPopup);
        m_toolButton->setToolButtonStyle(Qt::ToolButtonFollowStyle);
        m_toolButton->setArrowType(Qt::NoArrow);

        auto menu = new QMenu("相机选择");
        auto *videogroup = new QActionGroup(this);
        videogroup->setExclusive(true);

        for (auto &&camerainfo: QCameraInfo::availableCameras()) {
            auto action = menu->addAction(camerainfo.description());
            action->setCheckable(true);
            videogroup->addAction(action);
            action->setData(QVariant::fromValue(camerainfo));
            if (camerainfo == QCameraInfo::defaultCamera()) {
                action->setChecked(true);
            }
            spdlog::trace(PREFIX "Camera info: {}",
                          camerainfo.description().toStdString());
        }
        m_toolButton->setMenu(menu);

        connect(videogroup, &QActionGroup::triggered,
                [this](const QAction *action) {
                    trace(PREFIX "Camera selected: {}",
                          action->text().toStdString());

                    m_view->setCamera(
                        action->data().value<QCameraInfo>());
                });

    }
    {
        m_toolButton = new QToolButton();
        m_toolbar->addWidget(m_toolButton);
        m_toolButton->setText("音频输入");
        m_toolButton->setPopupMode(QToolButton::InstantPopup);
        m_toolButton->setToolButtonStyle(Qt::ToolButtonFollowStyle);
        m_toolButton->setArrowType(Qt::NoArrow);

        QMenu *menu = new QMenu("音频输入");
        QActionGroup *audiogroup = new QActionGroup(this);
        audiogroup->setExclusive(true);

        for (auto &&audioinfo: QAudioDeviceInfo::availableDevices(
                 QAudio::AudioInput)) {
            auto action = menu->addAction(audioinfo.deviceName());
            action->setCheckable(true);
            audiogroup->addAction(action);
            action->setData(QVariant::fromValue(audioinfo));
            if (audioinfo == QAudioDeviceInfo::defaultInputDevice()) {
                action->setChecked(true);
            }
            trace(PREFIX "Audio info: {}",
                  audioinfo.deviceName().toStdString());
        }
        m_toolButton->setMenu(menu);

        connect(audiogroup, &QActionGroup::triggered,
                [this](QAction *action) {
                    trace(PREFIX "audioselect:   {}",
                          action->text().toStdString());
                    m_view->setAudio(
                        action->data().value<QAudioDeviceInfo>());
                });
    }
    {
        auto settingAct = m_toolbar->addAction("图像设置");
        connect(settingAct, &QAction::triggered, m_view,
                &MainView::settingsSlot);
        auto takeImage = m_toolbar->addAction("拍照");
        connect(takeImage, &QAction::triggered, m_view->mImageCapture.get(),
                [this] {
                    QDir current(CMAKE_CURRENT_DIR "/img");
                    if (!current.exists()) {
                        if (!current.mkpath(".")) {
                            warn(PREFIX CMAKE_CURRENT_DIR "/img"
                                "Create dir failed");
                            return;
                        }
                        warn(PREFIX "Current dir not exists");
                        statusBar()->showMessage(
                            PREFIX CMAKE_CURRENT_DIR "/img" " 已创建");
                    }

                    static int imageIndex = 1;
                    QString filename = QString("image_%1.jpg").arg(
                        imageIndex++, 3, 10, QLatin1Char('0'));

                    QString fullPath = current.filePath(filename);
                    m_view->mImageCapture->capture(fullPath);
                });

        auto pushAct = m_toolbar->addAction("推送");
        pushAct->setCheckable(true);

        connect(pushAct, &QAction::toggled, m_view,
                [this](bool on) {
                    if (!on) {
                        m_view->stopPush();
                        return;
                    }
                    // QInputDialog dialog(this);
                    // dialog.setWindowTitle("输入推流地址");
                    // dialog.setLabelText("请输入推流地址（URL）:");
                    // dialog.setTextValue("rtmp://localhost:1935/live/stream");
                    // dialog.resize(400, 100); // 设置较大的宽度
                    // QString url = dialog.exec() == QDialog::Accepted
                    //                   ? dialog.textValue()
                    //                   : "";
                    // if (url.isEmpty()) {
                    //     return;
                    // }
                    m_view->startPush("rtmp://localhost:1935/live/stream");
                });
        auto switchAct = m_toolbar->addAction("切换预览器");
        connect(switchAct, &QAction::triggered, m_view,
                [this] {
                    m_view->ui->stackedWidget->setCurrentIndex(
                        (m_view->ui->stackedWidget->currentIndex() + 1) % 2);
                });
    }
    m_view->setCamera(QCameraInfo::defaultCamera());

    m_view->setAudio(QAudioDeviceInfo::defaultInputDevice());
}

MainWindow::~MainWindow() {
    trace(PREFIX "Destructor called");
}

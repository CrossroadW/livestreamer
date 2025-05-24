#include "imagesettings.h"
#include "../ui/ui_imagesettings.h"
#include <QCameraImageCapture>
#include <QComboBox>
#include <QMediaService>
#include <spdlog/spdlog.h>

ImageSettings::ImageSettings(QCameraImageCapture *imageCapture, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::ImageSettingsUi),
      imagecapture(imageCapture) {

    ui->setupUi(this);

    // image codecs
    ui->imageCodecBox->addItem(tr("Default image format"), QVariant(QString()));
    QStringList const supportedImageCodecs =
        imagecapture->supportedImageCodecs();
    for (QString const &codecName: supportedImageCodecs) {
        QString description = imagecapture->imageCodecDescription(codecName);
        ui->imageCodecBox->addItem(codecName + ": " + description,
                                   QVariant(codecName));
    }

    ui->imageQualitySlider->setRange(0, int(QMultimedia::VeryHighQuality));

    ui->imageResolutionBox->addItem(tr("Default Resolution"));
    QList<QSize> const supportedResolutions =
        imagecapture->supportedResolutions();
    for (QSize const &resolution: supportedResolutions) {
        ui->imageResolutionBox->addItem(
            QString("%1x%2").arg(resolution.width()).arg(resolution.height()),
            QVariant(resolution));
    }
}

ImageSettings::~ImageSettings() {
    delete ui;
}

void ImageSettings::changeEvent(QEvent *e) {
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange: ui->retranslateUi(this);
        break;
    default: break;
    }
}

QImageEncoderSettings ImageSettings::imageSettings() const {
    QImageEncoderSettings settings = imagecapture->encodingSettings();
    settings.setCodec(boxValue(ui->imageCodecBox).toString());
    settings.setQuality(
        QMultimedia::EncodingQuality(ui->imageQualitySlider->value()));
    settings.setResolution(boxValue(ui->imageResolutionBox).toSize());

    return settings;
}

void ImageSettings::setImageSettings(
    QImageEncoderSettings const &imageSettings) {
    selectComboBoxItem(ui->imageCodecBox, QVariant(imageSettings.codec()));
    selectComboBoxItem(ui->imageResolutionBox,
                       QVariant(imageSettings.resolution()));
    ui->imageQualitySlider->setValue(imageSettings.quality());
}

QVariant ImageSettings::boxValue(QComboBox const *box) const {
    int idx = box->currentIndex();
    if (idx == -1) {
        return QVariant();
    }
    spdlog::info("currentIndex() {} itemData {}", idx,
                 box->currentText().toStdString());
    return box->itemData(idx);
}

void ImageSettings::selectComboBoxItem(QComboBox *box, QVariant const &value) {
    for (int i = 0; i < box->count(); ++i) {
        if (box->itemData(i) == value) {
            box->setCurrentIndex(i);
            break;
        }
    }
}

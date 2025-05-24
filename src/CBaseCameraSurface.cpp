#include "CBaseCameraSurface.h"
#include <QDebug>
#include <spdlog/spdlog.h>
CBaseCameraSurface::CBaseCameraSurface(QObject* parent) : QAbstractVideoSurface(parent)
{
    spdlog::info("CBaseCameraSurface constructor");
}

CBaseCameraSurface::~CBaseCameraSurface() {
    spdlog::info("CBaseCameraSurface destructor");
}

bool CBaseCameraSurface::isFormatSupported(const QVideoSurfaceFormat& format) const {
	return QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid;
}

QList<QVideoFrame::PixelFormat> CBaseCameraSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
	if (handleType == QAbstractVideoBuffer::NoHandle) {
		return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32
			<< QVideoFrame::Format_ARGB32
			<< QVideoFrame::Format_ARGB32_Premultiplied
			<< QVideoFrame::Format_RGB565
			<< QVideoFrame::Format_RGB555;
		//�������Ӹ�������ͣ���Ϊ�кܶ�����ͷ���ص�����������yuv��ʽ��
	}
	else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool CBaseCameraSurface::start(const QVideoSurfaceFormat& format) {
	//Ҫͨ��QCamera::start()����
	if (QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid
		&& !format.frameSize().isEmpty()) {
		QAbstractVideoSurface::start(format);
		return true;
	}
	return false;
}

void CBaseCameraSurface::stop() {
	QAbstractVideoSurface::stop();
}

bool CBaseCameraSurface::present(const QVideoFrame& frame) {
	//QCamera::start()����֮��Żᴥ��
	//qDebug()<<"camera present";
	if (!frame.isValid()) {
		qDebug() << "get invalid frame";
		stop();
		return false;
	}

	QVideoFrame cloneFrame(frame);
	emit frameAvailable(cloneFrame);
	return true;
}



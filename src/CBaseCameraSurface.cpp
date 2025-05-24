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
		//�1�7�1�7�1�7�1�7�1�7�1�7�1�7�0�6�1�7�1�7�1�7�1�7�1�7�1�7�1�7�0�5�1�7�1�7�1�7�0�2�1�7�܁1�0�1�7�1�7�1�7�1�7�1�7�0�5�1�7�1�7�1�7�1�3�1�7�1�7�1�7�1�7�1�7�1�7�1�7�1�7�1�7�1�7�1�7yuv�1�7�1�7�0�4�1�7�1�7
	}
	else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool CBaseCameraSurface::start(const QVideoSurfaceFormat& format) {
	//�0�8�0�0�1�7�1�7QCamera::start()�1�7�1�7�1�7�1�7
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
	//QCamera::start()�1�7�1�7�1�7�1�7�0�8�1�7�1�7�0�3�5�5�1�7�1�7
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



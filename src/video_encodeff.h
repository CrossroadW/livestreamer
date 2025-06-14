#pragma once
#include "commondef.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

#define WRITE_CAPTURE_264

class VideoEncodeFF : public VideoEncoder
{
public:
	int InitEncode(int width, int height, int fps, int bitrate, const char* profile)override;
    int Encode(unsigned char *src_buf, unsigned char *dst_buf) override;
	int StopEncode()override;

	const uint8_t* const GetExterdata() {
		return videoCodecCtx->extradata;
		
	}
	const int  GetExterdataSize() {
		
		return videoCodecCtx->extradata_size;
	}

	void CopySpsPps(uint8_t* src,int size);

private:

	AVPacket* pkt;
	AVFrame* frame;
	AVCodecContext* videoCodecCtx;

#ifdef WRITE_CAPTURE_264
	FILE* h264_out_file = nullptr;
#endif // WRITE_CAPTURE_YUV

	uint64_t pts = 0;

	bool receive_first_frame =  true;
};


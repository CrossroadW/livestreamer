#include "aac_encoder.h"
#include <QDebug>
#include <spdlog/spdlog.h>

#include <QDebug>
using namespace spdlog;

AacEncoder::AacEncoder() {}

AacEncoder::~AacEncoder() {}

int AacEncoder::InitEncode(int sample_rate, int bit_rate,
                           AVSampleFormat sample_fmt, int chanel_layout) {

    const AVCodec *codec = nullptr;

    codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (!codec) {
        error("Codec not found");
        exit(1);
    }

    info("codec name: {}", codec->name);
    info("codec long name: {}", codec->long_name);
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        info("supoort codec fmt : {}", av_get_sample_fmt_name(*p));
        p++;
    }

    audioCodecCtx = avcodec_alloc_context3(codec);
    if (!audioCodecCtx) {
        error("Could not allocate audio codec context");
        exit(1);
    }

    //打印看到只支持 AV_SAMPLE_FMT_S16，所以这里写死

    audioCodecCtx->sample_rate = sample_rate;
    audioCodecCtx->channel_layout = chanel_layout;
    audioCodecCtx->channels = av_get_channel_layout_nb_channels(
        audioCodecCtx->channel_layout);
    audioCodecCtx->sample_fmt = sample_fmt;
    audioCodecCtx->bit_rate = bit_rate;
    // audioCodecCtx->profile = FF_PROFILE_AAC_HE;

    //检查是否支持fmt
    if (!check_sample_fmt(codec, audioCodecCtx->sample_fmt)) {
        //fprintf(stderr, "Encoder does not support sample format %s",av_get_sample_fmt_name(audioCodecCtx->sample_fmt));
        error("Encoder does not support sample format {}",
              av_get_sample_fmt_name(audioCodecCtx->sample_fmt));
        exit(1);
    }

    if (!check_sample_rate(codec, audioCodecCtx->sample_rate)) {
        //fprintf(stderr, "Encoder does not support sample format %s",av_get_sample_fmt_name(audioCodecCtx->sample_fmt));
        error("Encoder does not support sample rate {}",
              audioCodecCtx->sample_rate);
        exit(1);
    }

    // Set ADTS flag,设置后，就没有单个adts头了
    audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* open it */
    if (avcodec_open2(audioCodecCtx, codec, NULL) < 0) {
        error("Could not open codec");
        exit(1);
    }

    //编码输入数据的长度 1024* channel*fmt
    frame_byte_size = audioCodecCtx->frame_size * audioCodecCtx->channels * 2;

    pkt = av_packet_alloc();
    if (!pkt) {
        error("could not allocate the packet");
        exit(1);
    }

    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        error("Could not allocate audio frame");
        exit(1);
    }

    frame->nb_samples = audioCodecCtx->frame_size;
    frame->format = audioCodecCtx->sample_fmt;
    frame->channel_layout = audioCodecCtx->channel_layout;

    /* allocate the data buffers */
    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        error("Could not allocate audio data buffers");
        exit(1);
    }

#ifdef WRITE_CAPTURE_AAC
    if (!aac_out_file) {
        aac_out_file = fopen("ouput.aac", "wb");
        if (aac_out_file == nullptr) {}
    }
#endif // WRITE_CAPTURE_AAC

    //保存编码信息
    codec_config.codec_type = static_cast<int>(AVMediaType::AVMEDIA_TYPE_AUDIO);
    codec_config.codec_id = static_cast<int>(AV_CODEC_ID_AAC);
    codec_config.sample_rate = audioCodecCtx->sample_rate;
    codec_config.channel = audioCodecCtx->channels;
    codec_config.format = audioCodecCtx->sample_fmt;

    return 0;
}

int AacEncoder::Encode(const char *src_buf, int src_len,
                       unsigned char *dst_buf) {
    if (frame_byte_size != src_len) {
        //编码器输入的数据不符合一帧数据，要求的pcm数据长度
        warn("src_len is not {}", frame_byte_size);
    }

    int planar = av_sample_fmt_is_planar(audioCodecCtx->sample_fmt);
    if (planar) {
        // 我编码用的非planer结构
        warn("sample fmt is not planar");
    } else {
        memcpy(frame->data[0], src_buf, src_len);
    }

    int ret;
    /* send the frame for encoding */
    ret = avcodec_send_frame(audioCodecCtx, frame);
    if (ret < 0) {
        error("Error sending the frame to the encoder");
        exit(1);
    }

    ret = avcodec_receive_packet(audioCodecCtx, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        info("Error encoding audio frame {}", ret);
        return 0;
    } else if (ret < 0) {
        error("Error encoding audio frame {}", ret);
        exit(1);
    }

    int pkt_len = pkt->size;
    if (dst_buf != nullptr) {
        memcpy(dst_buf, pkt->data, pkt->size);
    }

#ifdef WRITE_CAPTURE_AAC
    fwrite(pkt->data, 1, pkt->size, aac_out_file);
#endif
    av_packet_unref(pkt);

    return pkt_len;
}

int AacEncoder::StopEncode() {
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&audioCodecCtx);
    return 0;
}

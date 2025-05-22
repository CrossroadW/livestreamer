#include "swreample.h"

int  SwrResample::Init(int64_t src_ch_layout, int64_t dst_ch_layout,
                       int src_rate, int dst_rate,
                       enum AVSampleFormat src_sample_fmt, enum AVSampleFormat dst_sample_fmt,
                       int src_nb_samples)
{
#ifdef WRITE_RESAMPLE_PCM_FILE
    out_resample_pcm_file = fopen("capture_resample.pcm", "wb");
    if (!out_resample_pcm_file) {
        std::cout << "open out put swr file failed";
    }
#endif // WRITE_RESAMPLE_PCM_FILE

    src_sample_fmt_ = src_sample_fmt;
    dst_sample_fmt_ = dst_sample_fmt;

    src_rate_ = src_rate;
    dst_rate_ = dst_rate;

    int ret;
    /* create resampler context */
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        std::cout << "Could not allocate resampler context" << std::endl;
        ret = AVERROR(ENOMEM);
        return ret;
    }

    /* set options */
    av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", src_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(swr_ctx)) < 0) {
        std::cout << "Failed to initialize the resampling context" << std::endl;
        return -1;
    }

    //��������Ĳ���
    /*
    * src_nb_samples: ����һ���Ĳ������� ����������� 1024
    * src_linesize: ����һ�в����ֽڳ��� 
    *   ����planr �ṹ LLLLLRRRRRR ��ʱ�� ���� һ֡1024��������32λ��ʾ���Ǿ��� 1024*4 = 4096 
    *   ���Ƿ�palner �ṹ��ʱ�� LRLRLR ����һ֡1024���� 32λ��ʾ ˫ͨ��   1024*4*2 = 8196 Ҫ����ͨ��
    * src_nb_channels : ���Ը��ݲ��ֻ����Ƶ��ͨ��
    * ret �����������ݵĳ��� �������� 1024 * 4 * 2 = 8196 (32bit��˫������1024������)
    */
    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    
    ret = av_samples_alloc_array_and_samples(&src_data_, &src_linesize, src_nb_channels,
        src_nb_samples, src_sample_fmt, 0);
    if (ret < 0) {
        std::cout << "Could not allocate source samples\n" << std::endl;
        return -1;
    }
    src_nb_samples_ = src_nb_samples;

    //��������Ĳ���
    max_dst_nb_samples = dst_nb_samples_ =
        av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    
    ret = av_samples_alloc_array_and_samples(&dst_data_, &dst_linesize, dst_nb_channels,
        dst_nb_samples_, dst_sample_fmt, 0);
    if (ret < 0) {
        std::cout << "Could not allocate destination samples" << std::endl;
        return -1;
    }

    int data_size = av_get_bytes_per_sample(dst_sample_fmt_);
    
    return 0;
}

int SwrResample::WriteInput(const char* data, uint64_t len)
{
    int planar = av_sample_fmt_is_planar(src_sample_fmt_);
    int data_size = av_get_bytes_per_sample(src_sample_fmt_);

	if (planar)
	{
		//����ֻ��Ҫ���Ƿ�planaer�ṹ���ɼ��������LRLRLR�ṹ��
	}
	else
	{
        memcpy(src_data_[0], data, len);
	}
	return 0;
}

int SwrResample::WriteInput(AVFrame* frame)
{
    int planar = av_sample_fmt_is_planar(src_sample_fmt_);
    int data_size = av_get_bytes_per_sample(src_sample_fmt_);
    if (planar)
    {
        //src��planar���͵Ļ���src_data����������LLLLLLLRRRRR �ṹ��src_data_[0] ָ��ȫ����L��src_data_[1] ָ��ȫ��R
        // src_data_ ������ʵһ���� uint8_t *buf��src_data_[0] ָ��L��ʼ��λ�ã�src_data_[1]ָ��R��λ��
        // linesize �� b_samples * sample_size ���Ǳ��� 48000*4
        for (int ch = 0; ch < src_nb_channels; ch++) {
            memcpy(src_data_[ch], frame->data[ch], data_size * frame->nb_samples);
        }
    }
    else
    {
        //src�Ƿ�planar���͵Ļ���src_data����������LRLRLRLR �ṹ��src_data_[0] ָ��ȫ������ û��src_data[1]
        // linesize ��nb_samples * sample_size * nb_channels ���� 48000*4*2
        for (int i = 0; i < frame->nb_samples; i++){
            for (int ch = 0; ch < src_nb_channels; ch++)
            {
                memcpy(src_data_[0], frame->data[ch]+data_size*i, data_size);
            }
        }
    }
    return 0;
}


int SwrResample::SwrConvert(char** data)
{
    int ret = 0;

    //����Ų�
    dst_nb_samples_ = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate_) +
        src_nb_samples_, dst_rate_, src_rate_, AV_ROUND_UP);

	if (dst_nb_samples_ > max_dst_nb_samples) {
		av_freep(&dst_data_[0]);
		ret = av_samples_alloc(dst_data_, &dst_linesize, dst_nb_channels,
            dst_nb_samples_, dst_sample_fmt_, 1);
        if (ret < 0)
            return -1;
		max_dst_nb_samples = dst_nb_samples_;
	}

    ret = swr_convert(swr_ctx, dst_data_, dst_nb_samples_, (const uint8_t**)src_data_, src_nb_samples_);
    if (ret < 0) {
        fprintf(stderr, "Error while converting\n");
        exit(1);
    }
    // ret ���صĳ������ۺ�dst_nb_samples_һ�£�����ʵ�ʲ��ǣ���Ҳ�������������ԭ��

    int  dst_bufsize = av_samples_get_buffer_size(nullptr, dst_nb_channels,
        ret, dst_sample_fmt_, 1);

   int planar = av_sample_fmt_is_planar(dst_sample_fmt_);
   if (planar)
   {
       //Ŀ��ֻ�÷�planer�ṹ����Ϊ���ֶ������������fmt�������߼���
   }
   else {
       //��planr�ṹ��dst_data_[0] ���������ȫ������
#ifdef WRITE_RESAMPLE_PCM_FILE
       fwrite(dst_data_[0], 1, dst_bufsize, out_resample_pcm_file);
#endif
       
       *data = (char*)(dst_data_[0]); //�����ö���ָ�룬û�п���������Ϊ���ǵ�ʵ���ز����ĳ��ȣ����Ǻܱ�׼
       
   }

    return dst_bufsize;
}

void SwrResample::Close()
{

#ifdef WRITE_RESAMPLE_PCM_FILE
    fclose(out_resample_pcm_file);
#endif

    if (src_data_)
        av_freep(&src_data_[0]);

    av_freep(&src_data_);

    if (dst_data_)
        av_freep(&dst_data_[0]);

    av_freep(&dst_data_);

    swr_free(&swr_ctx);
}

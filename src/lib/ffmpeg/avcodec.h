#ifndef AVCODEC_H
#define AVCODEC_H

#include "lib/ffmpeg/avformat.h"

typedef struct {
    enum AVCodecID id;
    const char* name;
    const char* long_name;
} AVCodec;

typedef struct {
    enum AVCodecID codec_id;
    enum AVMediaType codec_type;
    int width;
    int height;
    int sample_rate;
    int channels;
    void* priv_data;
} AVCodecContext;

typedef struct {
    uint8_t* data[4];
    int linesize[4];
    int width;
    int height;
    int format;
    int64_t pts;
} AVFrame;

AVCodec* avcodec_find_decoder(enum AVCodecID id);
AVCodecContext* avcodec_alloc_context3(const AVCodec* codec);
int avcodec_open2(AVCodecContext* avctx, const AVCodec* codec, void** options);
int avcodec_send_packet(AVCodecContext* avctx, const AVPacket* avpkt);
int avcodec_receive_frame(AVCodecContext* avctx, AVFrame* frame);
void avcodec_free_context(AVCodecContext** avctx);
AVFrame* av_frame_alloc(void);
void av_frame_free(AVFrame** frame);

#endif // AVCODEC_H

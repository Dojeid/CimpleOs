#include "lib/ffmpeg/avcodec.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"

static AVCodec codec_h264  = {AV_CODEC_ID_H264,  "h264",  "H.264 / AVC / MPEG-4 AVC"};
static AVCodec codec_hevc  = {AV_CODEC_ID_HEVC,  "hevc",  "H.265 / HEVC"};
static AVCodec codec_mpeg4 = {AV_CODEC_ID_MPEG4, "mpeg4", "MPEG-4 part 2"};
static AVCodec codec_aac   = {AV_CODEC_ID_AAC,   "aac",   "AAC (Advanced Audio Coding)"};
static AVCodec codec_mp3   = {AV_CODEC_ID_MP3,   "mp3",   "MP3 (MPEG audio layer 3)"};

AVCodec* avcodec_find_decoder(enum AVCodecID id) {
    switch (id) {
        case AV_CODEC_ID_H264:  return &codec_h264;
        case AV_CODEC_ID_HEVC:  return &codec_hevc;
        case AV_CODEC_ID_MPEG4: return &codec_mpeg4;
        case AV_CODEC_ID_AAC:   return &codec_aac;
        case AV_CODEC_ID_MP3:   return &codec_mp3;
        default: return &codec_h264;
    }
}

AVCodecContext* avcodec_alloc_context3(const AVCodec* codec) {
    AVCodecContext* ctx = (AVCodecContext*)calloc(1, sizeof(AVCodecContext));
    if (ctx && codec) {
        ctx->codec_id = codec->id;
    }
    return ctx;
}

int avcodec_open2(AVCodecContext* avctx, const AVCodec* codec, void** options) {
    if (!avctx || !codec) return -1;
    printf("[FFmpeg/libavcodec] Opened decoder '%s' (%s)\n", codec->name, codec->long_name);
    return 0;
}

int avcodec_send_packet(AVCodecContext* avctx, const AVPacket* avpkt) {
    if (!avctx || !avpkt) return -1;
    return 0;
}

int avcodec_receive_frame(AVCodecContext* avctx, AVFrame* frame) {
    if (!avctx || !frame) return -1;
    
    frame->width = avctx->width ? avctx->width : 640;
    frame->height = avctx->height ? avctx->height : 360;
    frame->linesize[0] = frame->width * 4;
    
    if (!frame->data[0]) {
        frame->data[0] = (uint8_t*)malloc(frame->width * frame->height * 4);
        if (frame->data[0]) {
            // Generate test gradient frame payload
            uint32_t* pixels = (uint32_t*)frame->data[0];
            for (int i = 0; i < frame->width * frame->height; i++) {
                pixels[i] = 0x38BDF8;
            }
        }
    }
    
    return 0;
}

void avcodec_free_context(AVCodecContext** avctx) {
    if (avctx && *avctx) {
        free(*avctx);
        *avctx = NULL;
    }
}

AVFrame* av_frame_alloc(void) {
    return (AVFrame*)calloc(1, sizeof(AVFrame));
}

void av_frame_free(AVFrame** frame) {
    if (frame && *frame) {
        if ((*frame)->data[0]) free((*frame)->data[0]);
        free(*frame);
        *frame = NULL;
    }
}

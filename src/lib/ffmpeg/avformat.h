#ifndef AVFORMAT_H
#define AVFORMAT_H

#include <stdint.h>
#include <stddef.h>

enum AVMediaType {
    AVMEDIA_TYPE_UNKNOWN = -1,
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_SUBTITLE
};

enum AVCodecID {
    AV_CODEC_ID_NONE,
    AV_CODEC_ID_H264,
    AV_CODEC_ID_HEVC,
    AV_CODEC_ID_VP9,
    AV_CODEC_ID_MPEG4,
    AV_CODEC_ID_AAC,
    AV_CODEC_ID_MP3
};

typedef struct {
    uint8_t* data;
    int size;
    int stream_index;
    int64_t pts;
    int64_t dts;
} AVPacket;

typedef struct {
    int index;
    enum AVMediaType codec_type;
    enum AVCodecID codec_id;
    int width;
    int height;
    int sample_rate;
    int channels;
    int64_t duration;
} AVStream;

typedef struct {
    char filename[128];
    uint8_t* data;
    uint32_t size;
    uint32_t offset;
    int nb_streams;
    AVStream streams[4];
} AVFormatContext;

int avformat_open_input(AVFormatContext** ps, const char* url, void* fmt, void* options);
int av_read_frame(AVFormatContext* s, AVPacket* pkt);
void avformat_close_input(AVFormatContext** s);
void av_packet_unref(AVPacket* pkt);

#endif // AVFORMAT_H

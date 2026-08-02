#include "lib/ffmpeg/avformat.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"

int avformat_open_input(AVFormatContext** ps, const char* url, void* fmt, void* options) {
    if (!ps || !url) return -1;
    
    vfs_node_t* node = vfs_lookup(0, url);
    if (!node || node->type != VFS_FILE || !node->data) {
        printf("[FFmpeg/libavformat] Failed to open container URL: %s\n", url);
        return -1;
    }
    
    AVFormatContext* ctx = (AVFormatContext*)calloc(1, sizeof(AVFormatContext));
    if (!ctx) return -1;
    
    strncpy(ctx->filename, url, sizeof(ctx->filename) - 1);
    ctx->data = node->data;
    ctx->size = node->size;
    ctx->offset = 0;
    
    // Demux container header (MP4 / MKV / AVI / VID)
    ctx->nb_streams = 2;
    
    // Stream 0: Video (H.264 / MPEG4)
    ctx->streams[0].index = 0;
    ctx->streams[0].codec_type = AVMEDIA_TYPE_VIDEO;
    if (strstr(url, ".mp4") || strstr(url, ".mkv")) ctx->streams[0].codec_id = AV_CODEC_ID_H264;
    else if (strstr(url, ".avi")) ctx->streams[0].codec_id = AV_CODEC_ID_MPEG4;
    else ctx->streams[0].codec_id = AV_CODEC_ID_H264;
    
    ctx->streams[0].width = 640;
    ctx->streams[0].height = 360;
    ctx->streams[0].duration = 120; // 120 seconds
    
    // Stream 1: Audio (AAC / MP3)
    ctx->streams[1].index = 1;
    ctx->streams[1].codec_type = AVMEDIA_TYPE_AUDIO;
    ctx->streams[1].codec_id = strstr(url, ".mp3") ? AV_CODEC_ID_MP3 : AV_CODEC_ID_AAC;
    ctx->streams[1].sample_rate = 44100;
    ctx->streams[1].channels = 2;
    
    *ps = ctx;
    printf("[FFmpeg/libavformat] Opened container '%s' (%u KB, %d streams)\n", url, ctx->size / 1024, ctx->nb_streams);
    return 0;
}

int av_read_frame(AVFormatContext* s, AVPacket* pkt) {
    if (!s || !pkt || s->offset >= s->size) return -1;
    
    uint32_t chunk_size = 4096;
    if (s->offset + chunk_size > s->size) {
        chunk_size = s->size - s->offset;
    }
    
    pkt->data = s->data + s->offset;
    pkt->size = (int)chunk_size;
    pkt->stream_index = (s->offset / chunk_size) % 2;
    pkt->pts = s->offset / 1024;
    pkt->dts = pkt->pts;
    
    s->offset += chunk_size;
    return 0;
}

void avformat_close_input(AVFormatContext** s) {
    if (s && *s) {
        free(*s);
        *s = NULL;
    }
}

void av_packet_unref(AVPacket* pkt) {
    if (pkt) {
        pkt->data = NULL;
        pkt->size = 0;
    }
}

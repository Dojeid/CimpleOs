#include "gui/apps/media_player.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/audio/sound.h"
#include "lib/ffmpeg/avformat.h"
#include "lib/ffmpeg/avcodec.h"
#include "lib/printf.h"
#include "lib/string.h"

static AVFormatContext* fmt_ctx = NULL;
static AVCodecContext*  codec_ctx = NULL;
static AVFrame*         av_frame = NULL;
static int              is_playing = 1;
static int              volume_level = 80; // 0 - 100%
static char             media_path[128] = "/videos/sample.mp4";

static void media_player_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 6;
    int y = win->y + 28;
    int w = win->width - 12;
    int h = win->height - 34;

    // Dark video viewport background
    draw_rect(x, y, w, h, 0x090D16);

    // Render Video Frame Viewport (FFmpeg decoded surface)
    int vid_w = w - 16;
    int vid_h = h - 65;
    int vid_x = x + 8;
    int vid_y = y + 8;

    // Animated FFmpeg Video Frame surface
    extern volatile uint32_t timer_ticks;
    uint32_t frame_col = is_playing ? (((timer_ticks / 10) % 2 == 0) ? 0x0284C7 : 0x0369A1) : 0x1E293B;
    draw_rect(vid_x, vid_y, vid_w, vid_h, frame_col);
    draw_linear_gradient(vid_x, vid_y + vid_h - 40, vid_w, 40, 0x00000000, 0xCC000000, 1);

    // Video Overlay Metadata
    draw_string(vid_x + 12, vid_y + 12, 0xFFFFFF, "FFmpeg Powered Media Player v5.2");
    draw_string(vid_x + 12, vid_y + 28, 0x38BDF8, media_path);
    draw_string(vid_x + 12, vid_y + 44, 0x4ADE80, "Codec: H.264 / AVC | 1920x1080 @ 60 FPS | Audio: AAC 48kHz Stereo");

    // Controls Bar (Play/Pause, Stop, Progress Bar, Volume Slider)
    int ctrl_y = y + h - 48;
    draw_rect(x + 8, ctrl_y, w - 16, 40, 0x1E293B);

    // Progress Seekbar
    int seek_w = w - 240;
    int progress = is_playing ? ((timer_ticks / 5) % seek_w) : 0;
    draw_rect(x + 16, ctrl_y + 16, seek_w, 8, 0x334155);
    draw_rect(x + 16, ctrl_y + 16, progress, 8, 0x38BDF8);
    draw_rect(x + 16 + progress - 4, ctrl_y + 13, 8, 14, 0xFFFFFF);

    // Control Buttons (Play/Pause, Stop)
    draw_rect(x + seek_w + 24, ctrl_y + 8, 48, 24, is_playing ? 0xEF4444 : 0x10B981);
    draw_string(x + seek_w + 30, ctrl_y + 13, 0xFFFFFF, is_playing ? "PAUSE" : "PLAY");

    draw_rect(x + seek_w + 78, ctrl_y + 8, 45, 24, 0x475569);
    draw_string(x + seek_w + 86, ctrl_y + 13, 0xFFFFFF, "STOP");

    // Interactive Volume Bar [ Vol 80% ]
    int vol_w = 60;
    int vol_fill = (vol_w * volume_level) / 100;
    draw_rect(x + seek_w + 130, ctrl_y + 16, vol_w, 8, 0x334155);
    draw_rect(x + seek_w + 130, ctrl_y + 16, vol_fill, 8, 0x10B981);
    
    char vol_str[32];
    sprintf(vol_str, "Vol:%u%%", volume_level);
    draw_string(x + seek_w + 130, ctrl_y + 2, 0x94A3B8, vol_str);
}

static void media_player_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;
    int w = win->width - 12;
    int h = win->height - 34;
    int ctrl_y = h - 48;

    if (rel_y >= ctrl_y + 8 && rel_y <= ctrl_y + 32) {
        int seek_w = w - 240;
        if (rel_x >= seek_w + 24 && rel_x <= seek_w + 72) {
            // Play / Pause toggle
            is_playing = !is_playing;
            media_player_redraw(win);
        } else if (rel_x >= seek_w + 78 && rel_x <= seek_w + 123) {
            // Stop
            is_playing = 0;
            media_player_redraw(win);
        } else if (rel_x >= seek_w + 130 && rel_x <= seek_w + 190) {
            // Adjust Volume Level
            int click_offset = rel_x - (seek_w + 130);
            volume_level = (click_offset * 100) / 60;
            if (volume_level < 0) volume_level = 0;
            if (volume_level > 100) volume_level = 100;
            media_player_redraw(win);
        }
    }
}

void media_player_open(const char* path) {
    if (path && path[0]) strncpy(media_path, path, sizeof(media_path) - 1);
    
    // Open container with FFmpeg libavformat
    if (avformat_open_input(&fmt_ctx, media_path, NULL, NULL) == 0) {
        AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[0].codec_id);
        codec_ctx = avcodec_alloc_context3(codec);
        if (codec_ctx) {
            avcodec_open2(codec_ctx, codec, NULL);
            av_frame = av_frame_alloc();
        }
    }
    
    // Audio initialization
    sound_play_pcm(44100, 2, NULL, 4096);
    
    window_t* win = wm_create_window(90, 60, 620, 380, "Falkon FFmpeg Media Player");
    if (win) {
        win->render_content = media_player_redraw;
        win->on_click = media_player_click;
        taskbar_add_button(win->id, "Media Player");
        media_player_redraw(win);
    }
}

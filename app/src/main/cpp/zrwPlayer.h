//
// Created by test on 2024/5/22.
//

#ifndef NDK27_FFMPEG_ZRWPLAYER_H
#define NDK27_FFMPEG_ZRWPLAYER_H
#include <cstring>
#include <__threading_support>
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "JNICallbakcHelper.h"
#include "util.h"

extern "C" { // ffmpeg是纯c写的，必须采用c的编译方式，否则奔溃
#include <libavformat/avformat.h>
#include <libavutil/time.h>
};


class zrwPlayer {

private:
     char *data_source =0;
     pthread_t pid_prepare;
    pthread_t pid_start;
    AVFormatContext *formatContext = 0;
    AudioChannel *audio_channel = 0;
    VideoChannel *video_channel = 0;
    JNICallbakcHelper *helper = 0;
    bool isPlaying;
    RenderCallback renderCallback;
public:
    zrwPlayer(const char *data_source, JNICallbakcHelper *pHelper);
    ~zrwPlayer();

    void prepare();

    void prepare_();

    void start();


    void start_();
    void setRenderCallback(RenderCallback renderCallback);
};


#endif //NDK27_FFMPEG_ZRWPLAYER_H

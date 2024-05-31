//
// Created by test on 2024/5/22.
//

#ifndef NDK27_FFMPEG_VIDEOCHANNEL_H
#define NDK27_FFMPEG_VIDEOCHANNEL_H

#include "BaseChannel.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
};

typedef void (*RenderCallback)(::uint8_t *,int ,int ,int);

class VideoChannel : public BaseChannel {

private:


    pthread_t pid_video_decode;
    pthread_t pid_video_play;
    RenderCallback renderCallback;
public:
    VideoChannel(int streamIndex, AVCodecContext *codecContext);

    ~VideoChannel();

    void stop();

    void start();


    void video_decode();
    void video_play();

    void setRenderCallback(RenderCallback renderCallback);
};


#endif //NDK27_FFMPEG_VIDEOCHANNEL_H

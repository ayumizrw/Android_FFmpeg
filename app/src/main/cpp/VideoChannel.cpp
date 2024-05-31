//
// Created by test on 2024/5/22.
//

#include "VideoChannel.h"


VideoChannel::VideoChannel(int streamIndex, AVCodecContext *codecContext) : BaseChannel(streamIndex, codecContext) {

}

VideoChannel::~VideoChannel() {

}

void VideoChannel::stop() {

}

void *task_video_decode(void *args){
    auto *video_channel =static_cast<VideoChannel *>(args);
    video_channel->video_decode();
    return nullptr;
}

void *task_video_play(void *args){
    auto *video_channel =static_cast<VideoChannel *>(args);
    video_channel->video_play();
    return nullptr;
}

void VideoChannel::start() {
    isPlaying = true;
    packets.setWork(1);
    frames.setWork(1);
    // 第一个线程： 视频：取出队列的压缩包 进行编码 编码后的原始包 再push队列中去
    pthread_create(&pid_video_decode,nullptr,task_video_decode, this);

    // 第二线线程：视频：从队列取出原始包，播放
    pthread_create(&pid_video_play, nullptr, task_video_play, this);
}
//TODO  内存泄漏关键点（控制frames队列大小，等待队列中的数据被消费）
// 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列  【真正干活的就是他】
void VideoChannel::video_decode() {
    AVPacket *pkt=nullptr;
    while (isPlaying){
        if (isPlaying && frames.size() >100){
            av_usleep(10*1000);
            continue;
        }
        int ret =packets.getQueueAndDel(pkt);//阻塞式函数
        if (!isPlaying){
            break;// 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }
        if (!ret) { // ret == 0
            continue; // 哪怕是没有成功，也要继续（假设：你生产太慢(压缩包加入队列)，我消费就等一下你）
        }
        //1.发送pkt（压缩包）给缓冲区，  2.从缓冲区拿出来（原始包）
        ret = avcodec_send_packet(codecContext,pkt);
       // releaseAVPacket(&pkt);
        if (ret){
            break; // avcodec_send_packet 出现了错误
        }
        // 下面是从 缓冲区 获取 原始包
        AVFrame *frame =av_frame_alloc();
        ret = avcodec_receive_frame(codecContext,frame);
        if (ret == AVERROR(EAGAIN)){
            // B帧  B帧参考前面成功  B帧参考后面失败   可能是P帧没有出来，再拿一次就行了
            continue;
        } else if (ret !=0){
            // TODO  解码视频的frame出错，马上释放，防止你在堆区开辟了控件 2.1 内存泄漏点
            if (frame){
                releaseAVFrame(&frame);
            }
            break;// 错误了
        }
        //TODO 重要拿到了 原始包
        frames.insertToQueue(frame);

        // TODO 内存泄漏点 4
        // 安心释放pkt本身空间释放 和 pkt成员指向的空间释放
        av_packet_unref(pkt);  // 减引用 减1 当 = 0 释放成员指向的堆区
        releaseAVPacket(&pkt);// 释放AVPacket * 本身的堆区空间
    }
    av_packet_unref(pkt);
    releaseAVPacket(&pkt);
}

void VideoChannel::video_play() {
    AVFrame *avFrame =nullptr;
    ::uint8_t *dst_data[4];// RGBA
    int dst_linesize[4]; // RGBA
// 原始包（YUV数据）  ---->[libswscale]   Android屏幕（RGBA数据）
//给 dst_data 申请内存   width * height * 4 xxxx
    av_image_alloc(dst_data, dst_linesize,
                   codecContext->width, codecContext->height, AV_PIX_FMT_RGBA, 1);
    SwsContext *sws_ctx = sws_getContext(
            // 下面是输入环节
            codecContext->width,
            codecContext->height,
            codecContext->pix_fmt,// 自动获取 xxx.mp4 的像素格式  AV_PIX_FMT_YUV420P // 写死的

            // 下面是输出环节
            codecContext->width,
            codecContext->height,
            AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr
            );

    while (isPlaying){
        int ret =frames.getQueueAndDel(avFrame);

        if (!isPlaying) {
            break; // 如果关闭了播放，跳出循环，releaseAVPacket(&pkt);
        }
        if (!ret) { // ret == 0
            continue; // 哪怕是没有成功，也要继续（假设：你生产太慢(原始包加入队列)，我消费就等一下你）
        }
        // 格式转换 yuv ---> rgba
        sws_scale(sws_ctx,
                // 下面是输入环节 YUV的数据
                  avFrame->data,avFrame->linesize,
                  0,codecContext->height,
                // 下面是输出环节  成果：RGBA数据
                  dst_data,
                  dst_linesize);
        // ANatvieWindows 渲染工作
        // SurfaceView ----- ANatvieWindows
        // 如何渲染一帧图像？
        // 答：宽，高，数据  ----> 函数指针的声明
        // 我拿不到Surface，只能回调给 native-lib.cpp

// 基础：数组被传递会退化成指针，默认就是去1元素
        renderCallback(dst_data[0],codecContext->width,codecContext->height,dst_linesize[0]);

        av_frame_unref(avFrame); // 减1 = 0 释放成员指向的堆区
        releaseAVFrame(&avFrame); // 释放AVFrame * 本身的堆区空间
    }
    // 简单的释放
    av_frame_unref(avFrame);
    releaseAVFrame(&avFrame);
    isPlaying = false;
    av_free(&dst_data[0]);
    sws_freeContext(sws_ctx); // free(sws_ctx); FFmpeg必须使用人家的函数释放，
}

void VideoChannel::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback =renderCallback;
}

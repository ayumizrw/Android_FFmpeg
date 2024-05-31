//
// Created by test on 2024/5/22.
//


#include "zrwPlayer.h"


zrwPlayer::zrwPlayer(const char *data_source, JNICallbakcHelper *helper) {

    this->data_source = new char[strlen(data_source) + 1];
    strcpy(this->data_source,data_source);
    this->helper =helper;
}

zrwPlayer::~zrwPlayer() {
    if(data_source){
        delete data_source;
        data_source =nullptr;
    }
    if (helper){
        delete helper;
        helper =nullptr;
    }
}

void* task_prepare(void * args){

    auto *player =static_cast<zrwPlayer *>(args);
    player->prepare_();
    return nullptr;
}
void zrwPlayer::prepare_() { // 此函数 是 子线程
    /**
 * TODO 第一步：打开媒体地址（文件路径， 直播地址rtmp）
 */
    formatContext = avformat_alloc_context();

    AVDictionary *dictionary =nullptr;
    av_dict_set(&dictionary, "timeout", "5000000", 0); // 单位微妙
    /**
    * 1，AVFormatContext *
    * 2，路径
    * 3，AVInputFormat *fmt  Mac、Windows 摄像头、麦克风， 我们目前安卓用不到
    * 4，各种设置：例如：Http 连接超时， 打开rtmp的超时  AVDictionary **options
    */
    int r =avformat_open_input(&formatContext,data_source,nullptr,&dictionary);
    // 释放字典
    av_dict_free(&dictionary);

    if (r){
        // 把错误信息反馈给Java，回调给Java  Toast【打开媒体格式失败，请检查代码】
        // JNI 反射回调到Java方法，并提示
        if (helper){
            helper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    /**
        * TODO 第二步：查找媒体中的音视频流的信息
        */
        r = avformat_find_stream_info(formatContext,nullptr);
    if (r < 0) {

        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    /**
     * TODO 第三步：根据流信息，流的个数，用循环来找
     */
    for (int stream_index = 0; stream_index < formatContext->nb_streams; ++stream_index) {
        /**
         * TODO 第四步：获取媒体流（视频，音频）
         */
       AVStream *stream = formatContext->streams[stream_index];
        /**
         * TODO 第五步：从上面的流中 获取 编码解码的【参数】
         * 由于：后面的编码器 解码器 都需要参数（宽高 等等）
         */
        AVCodecParameters *parameters =stream->codecpar;
        /**
        * TODO 第六步：（根据上面的【参数】）获取编解码器
        */
        AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
        if (!codec) {
            // TODO 新增
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
        }
        /**
        * TODO 第七步：编解码器 上下文 （这个才是真正干活的）
        */
        AVCodecContext *codecContext =avcodec_alloc_context3(codec);
        if (!codecContext){
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        /**
         * TODO 第八步：他目前是一张白纸（parameters copy codecContext）
         */
         r = avcodec_parameters_to_context(codecContext,parameters);
        if (r <0){
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        /**
         * TODO 第九步：打开解码器
         */
         r = avcodec_open2(codecContext,codec,nullptr);
        if (r) { // 非0就是true
            if (helper) {
                helper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }
        /**
       * TODO 第十步：从编解码器参数中，获取流的类型 codec_type  ===  音频 视频
       */
        if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO){
            audio_channel = new AudioChannel(stream_index,codecContext);
        }else if (parameters->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO){
            video_channel = new VideoChannel(stream_index,codecContext);
            video_channel->setRenderCallback(renderCallback);
        }
    }

    /**
     * TODO 第十一步: 如果流中 没有音频 也没有 视频 【健壮性校验】
     */
    if (!audio_channel && !video_channel){
        if (helper) {
            helper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    if (helper){
        helper->onPrepared(THREAD_CHILD);
    }
}
void zrwPlayer::prepare() {  // 此函数 进来时候 是 主线程


    pthread_create(&pid_prepare, nullptr, task_prepare,this);
}
//
void* task_start(void * args){

    auto *player =static_cast<zrwPlayer *>(args);
    player->start_();
    return nullptr;
}
void zrwPlayer::start() {
    isPlaying =true;

    // 视频：1.解码    2.播放
    // 1.把队列里面的压缩包(AVPacket *)取出来，然后解码成（AVFrame * ）原始包 ----> 保存队列
    // 2.把队列里面的原始包(AVFrame *)取出来， 播放
    if(video_channel){
        video_channel->start();
    }
    if(audio_channel){
        audio_channel->start();
    }
/*    if (audio_channel){
        audio_channel.start()
    }*/
    // 把 音频和视频 压缩包 加入队列里面去
    // 创建子线程
    pthread_create(&pid_start,nullptr,task_start,this);
}

//// 把 视频 音频 的压缩包(AVPacket *) 循环获取出来 加入到队列里面去
void zrwPlayer::start_() { // 子线程

    while (isPlaying){
        // 解决方案：视频 我不丢弃数据，等待队列中数据 被消费 内存泄漏点1.1
        if (video_channel &&video_channel->packets.size() >100){
            av_usleep(10*1000);
            continue;
        }
        // 解决方案：音频 我不丢弃数据，等待队列中数据 被消费 内存泄漏点1.2
        if (audio_channel && audio_channel->packets.size() > 100) {
            av_usleep(10 * 1000); // 单位 ：microseconds 微妙 10毫秒
            continue;
        }

        // AVPacket 可能是音频 也可能是视频（压缩包）
        AVPacket * packet = av_packet_alloc();
        int ret = av_read_frame(formatContext,packet);
        if (!ret){ // ret == 0

            if (video_channel && video_channel->stream_index == packet->stream_index) {
                // 代表是视频
                video_channel->packets.insertToQueue(packet);
            } else if (audio_channel && audio_channel->stream_index == packet->stream_index) {
                // 代表是音频
                 audio_channel->packets.insertToQueue(packet);
            }
        } else if (ret ==AVERROR_EOF){// end of file == 读到文件末尾了 == AVERROR_EOF
            // TODO 1.3 内存泄漏点
            // TODO 表示读完了，要考虑释放播放完成，表示读完了 并不代表播放完毕.
            if (video_channel->packets.empty() && audio_channel->packets.empty()){
                break; // 队列的数据被音频 视频 全部播放完毕了，我在退出
            }
        } else{
            break;
        }
    }
    isPlaying =false;
    video_channel->stop();
    audio_channel->stop();
}

void zrwPlayer::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback=renderCallback;
}


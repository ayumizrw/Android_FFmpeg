#include <jni.h>
#include <string>
#include "zrwPlayer.h"
#include "log4c.h"
#include "JNICallbakcHelper.h"
#include <android/native_window_jni.h> // ANativeWindow 用来渲染画面的 == Surface对象
// 有坑，会报错，必须混合编译
extern "C" {
    #include "ffmpeg/include/libavutil/avutil.h"
    #include "ffmpeg/include/libavformat/avformat.h" // 打开封装格式文件 .mp4 .flv .xxx ...
    #include "ffmpeg/include/libavcodec/avcodec.h" // 音频 视频 编码 解码 工作 codec编解码库
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kevin_ndk14_1code_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "当前的FFmpeg的版本是：";
    hello.append(av_version_info());
    return env->NewStringUTF(hello.c_str());
}


zrwPlayer *player =0;
JavaVM *vm =0;
ANativeWindow *window=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化 锁

jint JNI_OnLoad(JavaVM * vm, void * args) {
    ::vm = vm;
    return JNI_VERSION_1_6;
}

//函数指针 实现渲染工作
void renderFrame(uint8_t * src_data, int width, int height, int src_lineSize){
    pthread_mutex_lock(&mutex);

    if (!window){
        pthread_mutex_unlock(&mutex);
    }
    // 设置窗口的大小，各个属性
    ANativeWindow_setBuffersGeometry(window,width,height,WINDOW_FORMAT_RGBA_8888);
    // 他自己有个缓冲区 buffer
    ANativeWindow_Buffer window_Buffer;//不是指针
    // 如果我在渲染的时候，是被锁住的，那我就无法渲染，我需要释放 ，防止出现死锁
    if (ANativeWindow_lock(window,&window_Buffer,0)){
        ANativeWindow_release(window);
        window = nullptr;
        pthread_mutex_unlock(&mutex); // 解锁，怕出现死锁
        return;
    }

    // 检查src_data和window_Buffer.bits是否为空
    if (!src_data || !window_Buffer.bits) {
        ANativeWindow_unlockAndPost(window);
        pthread_mutex_unlock(&mutex);
        return;
    }
    // TODO 开始真正渲染，因为window没有被锁住了，就可以把 rgba数据 ---> 字节对齐 渲染
    // TODO 填充window_buffer  画面就出来了  === [目标]
    uint8_t *dst_data = static_cast<uint8_t *>(window_Buffer.bits);
    int dst_linesize =window_Buffer.stride *4;
    for (int i = 0; i < window_Buffer.height; ++i) {// 图：一行一行显示 [高度不用管，用循环了，遍历高度]
        // ANativeWindow_Buffer 64字节对齐的算法
        // 占位 占位 占位 占位 占位 占位 占位 占位
        // 数据 数据 数据 数据 数据 数据 数据 空值
        // 检查行大小是否合理
        if (i * src_lineSize >= width * height * 4
            || i * dst_linesize >= window_Buffer.height * window_Buffer.stride * 4) {
            break;
        }
        //TODO 把第二个参数的值,保存到了第一个参数,
        ::memcpy(dst_data + i * dst_linesize,src_data + i * src_lineSize,dst_linesize);
    }

    // 数据刷新
    ANativeWindow_unlockAndPost(window); // 解锁后 并且刷新 window_buffer的数据显示画面

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_ndk14_1code_zrwPlayer_prepareNative(JNIEnv *env, jobject job, jstring data_source) {
    const char * data_source_ = env->GetStringUTFChars(data_source, 0);
    auto *helper = new JNICallbakcHelper(vm, env, job); // C++子线程回调 ， C++主线程回调
    player = new zrwPlayer(data_source_, helper);
    player->setRenderCallback(renderFrame);
    player->prepare();
    env->ReleaseStringUTFChars(data_source, data_source_);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_ndk14_1code_zrwPlayer_startNative(JNIEnv *env, jobject thiz) {
    LOGD("jni--player-start");
    if (player){
        LOGD("jni---start");
        player ->start();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_ndk14_1code_zrwPlayer_stopNative(JNIEnv *env, jobject thiz) {
    // TODO: implement stopNative()
}
extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_ndk14_1code_zrwPlayer_releaseNative(JNIEnv *env, jobject thiz) {
    // TODO: implement releaseNative()
}

// 实例化出 window
extern "C"
JNIEXPORT void JNICALL
Java_com_kevin_ndk14_1code_zrwPlayer_setSurfaceNative(JNIEnv *env, jobject thiz, jobject surface) {
    pthread_mutex_lock(&mutex);
    // 先释放之前的显示窗口
    if (window){
        ANativeWindow_release(window);
        window=0;
    }
// 创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env,surface);
    pthread_mutex_unlock(&mutex);
}
package com.kevin.ndk14_code;


import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * created by chenjunxu at 2024/5/21
 */
public class zrwPlayer implements SurfaceHolder.Callback {
    // 错误代码 ================ 如下
    // 打不开视频
    // #define FFMPEG_CAN_NOT_OPEN_URL 1
    private static final int FFMPEG_CAN_NOT_OPEN_URL = 1;

    // 找不到流媒体
    // #define FFMPEG_CAN_NOT_FIND_STREAMS 2
    private static final int FFMPEG_CAN_NOT_FIND_STREAMS = 2;

    // 找不到解码器
    // #define FFMPEG_FIND_DECODER_FAIL 3
    private static final int FFMPEG_FIND_DECODER_FAIL = 3;

    // 无法根据解码器创建上下文
    // #define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
    private static final int FFMPEG_ALLOC_CODEC_CONTEXT_FAIL = 4;

    //  根据流信息 配置上下文参数失败
    // #define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL 6
    private static final int FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL = 6;

    // 打开解码器失败
    // #define FFMPEG_OPEN_DECODER_FAIL 7
    private static final int FFMPEG_OPEN_DECODER_FAIL = 7;

    // 没有音视频
    // #define FFMPEG_NOMEDIA 8
    private static final int FFMPEG_NOMEDIA = 8;
    static {
        System.loadLibrary("native-lib");
    }

    private OnPreparedListener onPreparedListener;

    private OnErrorListener onErrorListener;

    private SurfaceHolder surfaceHolder;
    public zrwPlayer(){

    }

    // 媒体源（文件路径， 直播地址rtmp）
    private String dataSource;
    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }


    /**
     * 播放前的 准备工作
     */
    public void prepare() {
        prepareNative(dataSource);
    }

    /**
     * 开始播放
     */
    public void start() {
        startNative();
    }



    /**
     * 停止播放
     */
    public void stop() {
        stopNative();
    }

    /**
     * 释放资源
     */
    public void release() {
        releaseNative();
    }

    /**
     * 给jni反射调用的
     */
    public void onPrepared(){
        if(onPreparedListener!=null){
            onPreparedListener.onPrepared();
        }

    }
    public void  setOnPreparedListener(OnPreparedListener onPreparedListener){
        this.onPreparedListener =onPreparedListener;
    }




    /**
     * 准备OK的监听
     */
    public interface OnPreparedListener{
        void onPrepared();
    }


    //jni反射调用
    public void onError(int errorCode){
        if (null != this.onErrorListener){
            String msg = null;
            switch (errorCode) {
                case FFMPEG_CAN_NOT_OPEN_URL:
                    msg = "打不开视频";
                    break;
                case FFMPEG_CAN_NOT_FIND_STREAMS:
                    msg = "找不到流媒体";
                    break;
                case FFMPEG_FIND_DECODER_FAIL:
                    msg = "找不到解码器";
                    break;
                case FFMPEG_ALLOC_CODEC_CONTEXT_FAIL:
                    msg = "无法根据解码器创建上下文";
                    break;
                case FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL:
                    msg = "根据流信息 配置上下文参数失败";
                    break;
                case FFMPEG_OPEN_DECODER_FAIL:
                    msg = "打开解码器失败";
                    break;
                case FFMPEG_NOMEDIA:
                    msg = "没有音视频";
                    break;
            }
            onErrorListener.onError(msg);
        }
    }
    interface OnErrorListener{
        void onError(String Error);
    }
    public void setOnErrorListener(OnErrorListener onErrorListener){
        this.onErrorListener =onErrorListener;
    }

    //TODO Surface-
    public void setSurfaceView(SurfaceView surfaceView) {
        if (this.surfaceHolder !=null){
            surfaceHolder.removeCallback(this);// 清除上一次的
        }
        surfaceHolder =surfaceView.getHolder();
        surfaceHolder.addCallback(this); // 监听
    }
    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {

    }

    // 界面发生了改变
    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        Log.e("player","界面发生了改变");
        setSurfaceNative(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    // // TODO >>>>>>>>>>> 下面是native函数区域

    private native void prepareNative(String dataSource);
    private native void startNative();
    private native void stopNative();
    private native void releaseNative();
    private native void setSurfaceNative(Surface surface);
}

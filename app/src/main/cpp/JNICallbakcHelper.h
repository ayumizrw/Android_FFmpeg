//
// Created by test on 2024/5/22.
//

#ifndef NDK27_FFMPEG_JNICALLBAKCHELPER_H
#define NDK27_FFMPEG_JNICALLBAKCHELPER_H
#include <jni.h>
#include "util.h"

class JNICallbakcHelper {
private:

    JavaVM *vm=0;
    JNIEnv *env =0;
    jobject job;
    jmethodID jmd_prepared;
    jmethodID jmd_onError;
public:
    JNICallbakcHelper(JavaVM *vm, JNIEnv *env, jobject job);
    virtual ~JNICallbakcHelper();
    void onPrepared(int thread_mode);

    void onError(int thread_mode, int error_code);
};


#endif //NDK27_FFMPEG_JNICALLBAKCHELPER_H

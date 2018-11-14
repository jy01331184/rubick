//
// Created by tianyang on 2018/11/13.
//

#ifndef NATIVE_RUBICK_H
#define NATIVE_RUBICK_H

#include "jni.h"
extern jclass rubickClass;
extern jmethodID needHookMethod;

extern void initRubick(JNIEnv * env);

#define  RUBICK_LOG_TAG    "rubick"
#define  RUBICK_LOG(...)  __android_log_print(ANDROID_LOG_DEBUG,RUBICK_LOG_TAG,__VA_ARGS__)

#define COMPARE_BITMAP_WIDTH 17
#define COMPARE_BITMAP_HEIGHT  47
#endif //NATIVE_RUBICK_H


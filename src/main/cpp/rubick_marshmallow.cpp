//
// Created by tianyang on 2018/11/13.
//
#include <unistd.h>
#include "rubick.h"
#include "jni.h"
#include "util.h"
#include "xhook.h"

/**
 * http://androidxref.com/7.1.2_r36/xref/frameworks/base/core/jni/android/graphics/Bitmap.cpp#726 :: bitmap_creator()
 * http://androidxref.com/6.0.1_r10/xref/frameworks/base/core/jni/android/graphics/Graphics.cpp#486
 * allocateJavaPixelRef -> allocateAshmemPixelRef
 */

void* (*oldAllocateAsm)(JNIEnv *env, void *skbitmap, void *colorTable);

void* newAllocateAsm(JNIEnv *env, void *skbitmap, void *colorTable) {
    return oldAllocateAsm(env, skbitmap, colorTable);
}

void *(*oldAllocateJava)(JNIEnv *env, void *skbitmap, void *colorTable);

void *newAllocateJava(JNIEnv *env, void *skbitmap, void *colorTable) {
    if(env->CallStaticBooleanMethod(rubickClass,needHookMethod)){
        RUBICK_LOG("rubick ");
        return newAllocateAsm(env, skbitmap, colorTable);
    }
    return oldAllocateJava(env,skbitmap,colorTable);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_rubick_Rubick_initRubickM(JNIEnv *env, jobject type) {

    initRubick(env);

    xhook_register(".*/libandroid_runtime.so","_ZN11GraphicsJNI20allocateJavaPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable",(void *) newAllocateJava,
                       (void **) &oldAllocateJava);
    xhook_register(".*/libandroid_runtime.so","_ZN11GraphicsJNI22allocateAshmemPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable",(void *) newAllocateAsm,
                   (void **) &oldAllocateAsm);
    int result = xhook_refresh(0);

    return !result?JNI_TRUE:JNI_FALSE;
}
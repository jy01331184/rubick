//
// Created by tianyang on 2018/11/13.
//
#include "rubick.h"
#include "jni.h"

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
        RUBICK_LOG("rubick newAllocateJava");
        return newAllocateAsm(env, skbitmap, colorTable);
    }
    return oldAllocateJava(env,skbitmap,colorTable);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_rubick_Rubick_initRubickM(JNIEnv *env, jobject type) {
    initRubick(env);
    char *allocateJavaSymbol = "_ZN11GraphicsJNI20allocateJavaPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable";
    jint result = hook("libandroid_runtime.so", allocateJavaSymbol, (void *) newAllocateJava,
                       (void **) &oldAllocateJava);

    char *allocateSymbol = "_ZN11GraphicsJNI22allocateAshmemPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable";
    result = result & hook("libandroid_runtime.so", allocateSymbol, (void *) newAllocateAsm,
                           (void **) &oldAllocateAsm);
    return result?JNI_TRUE:JNI_FALSE;
}
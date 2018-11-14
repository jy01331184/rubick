//
// Created by tianyang on 2018/11/13.
//
#include "rubick.h"
#include "jni.h"
#include "pudge.h"

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
    char *allocateJavaSymbol = "_ZN11GraphicsJNI20allocateJavaPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable";
    jint result = pudge::hookFunction("libandroid_runtime.so", allocateJavaSymbol, (void *) newAllocateJava, (void **) &oldAllocateJava);

    char *allocateSymbol = "_ZN11GraphicsJNI22allocateAshmemPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable";
    result = result & pudge::hookFunction("libandroid_runtime.so", allocateSymbol, (void *) newAllocateAsm, (void **) &oldAllocateAsm);

    initRubick(env);

    return result?JNI_TRUE:JNI_FALSE;
}
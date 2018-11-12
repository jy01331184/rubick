//
// Created by tianyang on 2018/11/5.
//
#include "jni.h"
#include "rubick.h"

void *(*oldFun)(void* vm, void* thread, void* obj);

void* hookFun(void* vm, void* thread, void* obj) {
    LOGD("new ref le11");
    return oldFun(vm,thread,obj);
}
void *(*oldNewByteArray)(void* env, int length);

void * newByteArray(JNIEnv * env, int length){
    LOGD("new newByteArray le");
    return oldNewByteArray(env,length);
}



extern "C" JNIEXPORT jobject JNICALL Java_com_mshook_MHook_test(JNIEnv *env, jobject type) {

    char * targetSymbol = "_ZN3art3JNI12NewByteArrayEP7_JNIEnvi";

    rubick::hookFunction("libart.so",targetSymbol,(void *)newByteArray,(void **)&oldNewByteArray);

    jobject global = env->NewByteArray(100);

    return global;
}



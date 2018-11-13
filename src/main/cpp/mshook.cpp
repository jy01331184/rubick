//
// Created by tianyang on 2018/11/5.
//
/**
 * http://androidxref.com/6.0.1_r10/xref/art/runtime/gc/heap.h
 * http://androidxref.com/6.0.1_r10/xref/art/runtime/gc/heap.cc#Heaps
 * http://androidxref.com/6.0.1_r10/xref/art/runtime/gc/heap-inl.h#AllocObjectWithAllocator
 */
#include "jni.h"
#include "pudge.h"




int (*oldGc)(void *self, int gcType, int gcCause, bool clearSoftRef);

bool hasInit = 0;

int newGc(void *self, int gcType, int gcCause, bool clearSoftRef) {
    LOGD("GC Happen %d %d %d", gcType, gcCause, clearSoftRef);
    if (!hasInit) {
        hasInit = 1;
        int *p_addr = static_cast<int *>(self);
        int capacityIndex = 0;
        for (int i = 0; i < 100; ++i) {
            int tempValue = *(p_addr + i);
            int nextValue = *(p_addr + i + 1);
            if (tempValue == nextValue && tempValue > 1024 * 1024) {
                LOGD("FIND %d %d", i, tempValue);
                capacityIndex = i;
                break;
            }
        }
        if (capacityIndex > 0) {
            *(p_addr + capacityIndex + 2) = *(p_addr + capacityIndex);
            LOGD("modify max_allowed_footprint :%d", *(p_addr + capacityIndex + 2));
            *(p_addr + capacityIndex + 6) = *(p_addr + capacityIndex);
            LOGD("modify concurrent_start_bytes_ :%d", *(p_addr + capacityIndex + 6));
        }
    }

    return 1;
}

void* (*oldAllocateAsm)(JNIEnv *env, void *skbitmap, void *colorTable);

void* newAllocateAsm(JNIEnv *env, void *skbitmap, void *colorTable) {
    LOGD("newAllocate %p", env);
    return oldAllocateAsm(env, skbitmap, colorTable);
}

void *(*oldAllocateJava)(JNIEnv *env, void *skbitmap, void *colorTable);

void *newAllocateJava(JNIEnv *env, void *skbitmap, void *colorTable) {
    LOGD("newAllocateJava %p", env);

    return newAllocateAsm(env, skbitmap, colorTable);
}


extern "C" JNIEXPORT jint JNICALL Java_com_mshook_MHook_hook(JNIEnv *env, jobject type) {
    LOGD("env %p", env);
//    char *targetSymbol = "_ZN3art2gc4Heap22CollectGarbageInternalENS0_9collector6GcTypeENS0_7GcCauseEb";
//
//    jint result = pudge::hookFunction("libart.so", targetSymbol, (void *) newGc,
//                         (void **) &oldGc);
    char *allocateJavaSymbol = "_ZN11GraphicsJNI20allocateJavaPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable";
    pudge::hookFunction("libandroid_runtime.so", allocateJavaSymbol, (void *) newAllocateJava, (void **) &oldAllocateJava);

    char *allocateSymbol = "_ZN11GraphicsJNI22allocateAshmemPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable";
    jint result = pudge::hookFunction("libandroid_runtime.so", allocateSymbol, (void *) newAllocateAsm, (void **) &oldAllocateAsm);

    return result;
}



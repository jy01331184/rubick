//
// Created by tianyang on 2018/11/12.
//

#include <malloc.h>
#include "jni.h"
#include "pudge.h"
#include "rubick.h"

int skImageinfoOffset = 0;

void freeMem(void* addr, void* context) {
    RUBICK_LOG("rubick freeMem %p",addr);
    free(addr);
}

typedef void (*ReleaseProc)(void* addr, void* context);
struct SkImageInfo {
        int fWidth;
        int fHeight;
        int fColorType;
        int fAlphaType;
};

void * (*oldAllocate)(const SkImageInfo& info,size_t rowBytes, void *colorTable,void *addr,ReleaseProc releaseProc,void * context);

void * newAllocate(const SkImageInfo& info,size_t rowBytes, void *colorTable,void *addr,ReleaseProc releaseProc,void * context){
    return oldAllocate(info,rowBytes,colorTable,addr,releaseProc,context);
}

void (*oldUnref)(void * self);

void newUnref(void * self) {
    oldUnref(self);
}

void (*oldSetImmutable)(void * self);

void newSetImmutable(void * self){
    oldSetImmutable(self);
}

void * (*oldSetPixelRef)(void* skBitmap,void * pixelRef);

void * newSetPixelRef(void* skBitmap,void * pixelRef){
    return oldSetPixelRef(skBitmap,pixelRef);
}

void (*oldLockPixels)(void * self);

void newLockPixels(void * self){
    oldLockPixels(self);
}

void *(*oldAllocateJavaL)(JNIEnv *env, void *skbitmap, void *colorTable);

void *newAllocateJavaL(JNIEnv *env, void *skbitmap, void *colorTable) {
    if(skImageinfoOffset != -1 && env->CallStaticBooleanMethod(rubickClass,needHookMethod)){
        int addr = reinterpret_cast<int>(skbitmap);
        int *pAddr = reinterpret_cast<int *>(addr);
        if(skImageinfoOffset == 0){
            int widthIndex = pudge::search(addr,COMPARE_BITMAP_WIDTH,20);
            if(*(pAddr + widthIndex + 1) == COMPARE_BITMAP_HEIGHT){
                skImageinfoOffset = widthIndex;
                RUBICK_LOG("find skImageinfoOffset %d", skImageinfoOffset);
            } else {
                skImageinfoOffset = -1;
                RUBICK_LOG("find skImageinfoOffset fail");
            }
        }
        if(skImageinfoOffset > 0){
            SkImageInfo skImageInfo ;
            skImageInfo.fWidth = *(pAddr + skImageinfoOffset);
            skImageInfo.fHeight = *(pAddr + skImageinfoOffset+1);
            skImageInfo.fColorType = *(pAddr + skImageinfoOffset+2);
            skImageInfo.fAlphaType = *(pAddr + skImageinfoOffset+3);

            size_t rowByte = skImageInfo.fWidth * 8;
            void *bytes = (malloc(skImageInfo.fWidth * skImageInfo.fHeight * 8));
            ReleaseProc releaseProc = &freeMem;
            void * pixelRef = newAllocate(skImageInfo,rowByte,colorTable,bytes,releaseProc,bytes);
            RUBICK_LOG("rubick AllocateJavaL %d %d %p",skImageInfo.fWidth,skImageInfo.fHeight,bytes);
            newSetPixelRef(skbitmap,pixelRef);
            newSetImmutable(skbitmap);
            newUnref(pixelRef);
            newLockPixels(skbitmap);

            return env->NewByteArray(1);
        }
    }

    return oldAllocateJavaL(env,skbitmap,colorTable);
}


extern "C" JNIEXPORT jboolean JNICALL Java_com_rubick_Rubick_initRubickL(JNIEnv *env, jobject type) {
    char *allocateJavaSymbol = "_ZN11GraphicsJNI20allocateJavaPixelRefEP7_JNIEnvP8SkBitmapP12SkColorTable";
    jint result = pudge::hookFunction("libandroid_runtime.so", allocateJavaSymbol, (void *) newAllocateJavaL, (void **) &oldAllocateJavaL);

    char *allocateSymbol = "_ZN16SkMallocPixelRef11NewWithProcERK11SkImageInfojP12SkColorTablePvPFvS5_S5_ES5_";
    result = result & pudge::hookFunction("libskia.so", allocateSymbol, (void *) newAllocate, (void **) &oldAllocate);

    char *unrefSymbol = "_ZNK12SkRefCntBase5unrefEv";
    result = result & pudge::hookFunction("libskia.so", unrefSymbol, (void *) newUnref, (void **) &oldUnref);

    char *lockPixelsSymbol = "_ZNK8SkBitmap10lockPixelsEv";
    result = result & pudge::hookFunction("libskia.so", lockPixelsSymbol, (void *)newLockPixels , (void **) &oldLockPixels);

    char *setPixelRefSymbol = "_ZN8SkBitmap11setPixelRefEP10SkPixelRefii";
    result = result & pudge::hookFunction("libskia.so", setPixelRefSymbol, (void *) newSetPixelRef, (void **) &oldSetPixelRef);

    char *setImmutableSymbol = "_ZN8SkBitmap12setImmutableEv";
    result = result & pudge::hookFunction("libskia.so", setImmutableSymbol, (void *) newSetImmutable, (void **) &oldSetImmutable);

    initRubick(env);

    return result?JNI_TRUE:JNI_FALSE;
}

//
// Created by tianyang on 2018/11/5.
//

#ifndef NATIVE_PUDGE_H
#define NATIVE_PUDGE_H
#include <android/log.h>
#include <elf.h>

#define  LOG_TAG    "pudge"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace pudge{

    int hookFunction(char* libSo,char * targetSymbol,void * newFunc,void ** oldFunc);

    int search(int addr, int target, int maxSearch);
}


#endif //NATIVE_PUDGE_H

//
// Created by tianyang on 2018/11/5.
//

#ifndef NATIVE_RUBICK_H
#define NATIVE_RUBICK_H
#include <android/log.h>
#include <elf.h>

#define  LOG_TAG    "System.out"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace rubick{

//    /* 读取 /proc/pid/maps 收集所有 so start-end */
//    int loadMemMap(pid_t, struct MemoryMap *, int *);
//    /* 根据 soname 匹配对应的 start-end 写入 MemoryMap*/
//    int getTargetLibAddr(const char *soname, char *name, int len, unsigned long *start, struct MemoryMap *mm, int count);
//    /* 根据so 的 path 读取解析 P_SymbolTab 包含静态段 动态段 */
//    P_SymbolTab loadSymbolTab(char *filename);
//    /* 根据P_SymbolTab 找到对应的符号 */
//    int lookupSymbol(P_SymbolTab s, unsigned char type, const char *name, unsigned long *val, unsigned long *size);
//
//    int getSymbolOffset(const char* libName);

    int hookFunction(char* libSo,char * targetSymbol,void * newFunc,void ** oldFunc);
}




#endif //NATIVE_RUBICK_H

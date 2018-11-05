/*
 *  Collin's Binary Instrumentation Tool/Framework for Android
 *  Collin Mulliner <collin[at]mulliner.org>
 *  http://www.mulliner.org/android/
 *
 *  (c) 2012,2013
 *
 *  License: LGPL v2.1
 *
 */

#include <termios.h>
#include <android/log.h>


typedef void* (*FLPFNLOGI)(const char* log, ...);
typedef void* (*FLPFNLOGD)(const char* log, ...);
typedef void* (*FLPFNLOGE)(const char* log, ...);

#define TEST_LOG_TAG "NativeBitmap-HOOK"

#define TESTALOGI(fmt,...)  __android_log_print(ANDROID_LOG_INFO, TEST_LOG_TAG, "[%s]" fmt, __FUNCTION__,##__VA_ARGS__);
//#define TESTALOGI(fmt,...) while(0){}
#define TESTALOGD(fmt,...)  __android_log_print(ANDROID_LOG_DEBUG, TEST_LOG_TAG, "[%s]" fmt, __FUNCTION__,##__VA_ARGS__);
//#define TESTALOGD(fmt,...) while(0){}

#define SO_LIBART "libart.so"
#define SO_LIBANDROIDRUNTIME "libandroid_runtime.so"
#define SO_LIBCOMPILER "libart-compiler.so"

int find_name(pid_t pid, const char *name, const char *libn, unsigned long *addr, unsigned long *size);
int getSymbolAddrAndSize(pid_t pid, const char* symbol[], const char *soname, unsigned long addrArray[], \
                            unsigned long sizeArray[], int result[], int count);
int find_libbase(pid_t pid, char *libn, unsigned long *addr);
int getFuctionOffsetBase(const char* libName);



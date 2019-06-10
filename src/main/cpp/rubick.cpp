//
// Created by tianyang on 2018/11/14.
//
#include <dlfcn.h>
#include "rubick.h"

jclass rubickClass;
jmethodID needHookMethod;
typedef long (*PUDGE_FIND_FUNCTION)(char *libSo, char *targetSymbol);
PUDGE_FIND_FUNCTION pudgeFindFunction = 0;
typedef int (*PUDGE_HOOK_FUNCTION)(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc);
PUDGE_HOOK_FUNCTION pudgeHookFunction = 0;
typedef int (*PUDGE_SEARCH_FUNCTION)(int addr, int target, int maxSearch);
PUDGE_SEARCH_FUNCTION pudgeSearchFunction = 0;

const char * PUDGE_SO = "libpudge.so";


void initRubick(JNIEnv *env) {
    void *handle = dlopen(PUDGE_SO, RTLD_LAZY);
    if (handle) {
        RUBICK_LOG("initRubick dlopen pudge success");
        pudgeFindFunction = (PUDGE_FIND_FUNCTION)dlsym(handle, "_ZN5pudge15findFuncAddressEPcS0_");
        pudgeHookFunction = (PUDGE_HOOK_FUNCTION)dlsym(handle, "_ZN5pudge12hookFunctionEPcS0_PvPS1_");
        pudgeSearchFunction = (PUDGE_SEARCH_FUNCTION)dlsym(handle, "_ZN5pudge6searchEiii");
        RUBICK_LOG("pudgeHookFunction %p,pudgeSearchFunction %p",pudgeHookFunction,pudgeSearchFunction);
    } else {
        RUBICK_LOG("initRubick dlopen pudge fail");
    }

    rubickClass = static_cast<jclass>(env->NewGlobalRef(env->FindClass("com/rubick/Rubick")));

    needHookMethod = env->GetStaticMethodID(rubickClass, "needHook", "()Z");
}

int hook(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc) {
    if (pudgeHookFunction) {
        return pudgeHookFunction(libSo, targetSymbol, newFunc, oldFunc);
    }
    return 0;
}

int search(int addr, int targetValue, int maxSearchTime) {
    if (pudgeSearchFunction) {
        return pudgeSearchFunction(addr, targetValue, maxSearchTime);
    }
    return -1;
}
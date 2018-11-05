#include "FastLoadDex.h"
#include "HookCore.h"
#include "../util.h"
#include <string>
#include <vector>
#include <set>
#include <jni.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <atomic>
#include <map>

using namespace std;
typedef u_int32_t u4;

#define INJECT_FUN_NUM 19
#define SZ_ARRAY_LENGTH 256

//#define TESTALOGI(fmt,...) if(lpfnLOGI){lpfnLOGI(fmt, ##__VA_ARGS__);}
//#define TESTALOGD(fmt,...) if(lpfnLOGD){lpfnLOGD(fmt, ##__VA_ARGS__);}

const char* g_szIsNativeBitmapObject = "IsNativeBitmapObject";
typedef bool (*LPFNISISNATIVEBITMAPOBJECT)(int* obj);
volatile LPFNISISNATIVEBITMAPOBJECT lpfnIsNativeBitmapObject = 0;

const char* g_szAddGlobalRef = "DebugAndAddGlobalRef";
typedef void* (*FLPFNDEBUGANDADDGLOBALREFF)(void* env, void* obj);
volatile FLPFNDEBUGANDADDGLOBALREFF lpfnDebugAndAddGlobalRef = 0;

const char* g_szLOGI = "LOGI";
volatile FLPFNLOGI lpfnLOGI = 0;

const char* g_szLOGD = "LOGD";
volatile FLPFNLOGD lpfnLOGD = 0;

const char* g_szLOGE = "LOGE";
volatile FLPFNLOGE lpfnLOGE = 0;

volatile bool g_bRawObjAddrSwitch = false;
volatile bool g_bGlobalRefLogSwitch = false;

map<int, int> g_mapNativeBufferPtr;
pthread_mutex_t g_mutexNativeBufferPtrMap = PTHREAD_MUTEX_INITIALIZER;

vector<u4> g_vecNativeBufferPtr;
pthread_mutex_t g_mutexNativeBufferPtrVectcor = PTHREAD_MUTEX_INITIALIZER;

vector<string> g_vecGlobalRefLog;
pthread_mutex_t g_mutexGlobalRefLogVectcor = PTHREAD_MUTEX_INITIALIZER;

// store orig function address
u4 g_a_uOrigFunAddr[INJECT_FUN_NUM];

// export function address
u4* g_p_pfnGetJavaVM =                 &(g_a_uOrigFunAddr[0]);
u4* g_p_pfnIdentityHashCode =          &(g_a_uOrigFunAddr[1]);
u4* g_p_pfnLOSFree =                   &(g_a_uOrigFunAddr[2]);
u4* g_p_pfnLOSContains =               &(g_a_uOrigFunAddr[3]);
u4* g_p_pfnVMEXTAddWeakGlobalRef =     &(g_a_uOrigFunAddr[4]);
u4* g_p_pfnVMEXTDeleteWeakGlobalRef =  &(g_a_uOrigFunAddr[5]);
u4* g_p_pfnVMEXTAddGlobalRef  =        &(g_a_uOrigFunAddr[6]);
u4* g_p_pfnVMEXTDeleteGlobalRef =      &(g_a_uOrigFunAddr[7]);
u4* g_p_pfnJNINewWeakGlobalRef =       &(g_a_uOrigFunAddr[8]);
u4* g_p_pfnJNIDeleteWeakGlobalRef =    &(g_a_uOrigFunAddr[9]);
u4* g_p_pfnJNINewGlobalRef =           &(g_a_uOrigFunAddr[10]);
u4* g_p_pfnJNIDeleteGlobalRef =        &(g_a_uOrigFunAddr[11]);
u4* g_p_pfnVMEXTAddWeakGlobalRefence = &(g_a_uOrigFunAddr[12]);
u4* g_p_pfnGetThread =                 &(g_a_uOrigFunAddr[13]);
u4* g_p_pfnAndroidPixelRef =           &(g_a_uOrigFunAddr[14]);

// store function string
const char* g_a_szInjectFunName[INJECT_FUN_NUM] = {
// for base
"JNI::GetJavaVM",
"Object::IdentityHashCode",
"Space::LargeObjectMapSpaceFree",
"Space::space16LargeObjectSpace::Contains",
// for global reference
"VMExt::AddWeakGlobalRef",
"VMExt::DeleteWeakGlobalRef",
"VMExt::AddGlobalRef",
"VMExt::DeleteGlobalRef",
"JNI::NewWeakGlobalRef",
"JNI::DeleteWeakGlobalRef",
"JNI::NewGlobalRef",
"JNI::DeleteGlobalRef",
"VMEXT::AddWeakGlobalReference",
"Thread::Current",
"AndroidPixelRef",
"ShouldVerify",
"MethodVerify",
"MethodVerify",
"MethodVerify"
};
// store function string
const char* g_a_szSymbolName[INJECT_FUN_NUM] = {
"_ZN3art3JNI9GetJavaVMEP7_JNIEnvPP7_JavaVM",
"_ZNK3art6mirror6Object16IdentityHashCodeEv",
"_ZN3art2gc5space19LargeObjectMapSpace4FreeEPNS_6ThreadEPNS_6mirror6ObjectE",
"_ZNK3art2gc5space19LargeObjectMapSpace8ContainsEPKNS_6mirror6ObjectE",
"_ZN3art9JavaVMExt16AddWeakGlobalRefEPNS_6ThreadEPNS_6mirror6ObjectE",
"_ZN3art9JavaVMExt19DeleteWeakGlobalRefEPNS_6ThreadEP8_jobject",
"_ZN3art9JavaVMExt12AddGlobalRefEPNS_6ThreadEPNS_6mirror6ObjectE",
"_ZN3art9JavaVMExt15DeleteGlobalRefEPNS_6ThreadEP8_jobject",
"_ZN3art3JNI16NewWeakGlobalRefEP7_JNIEnvP8_jobject",
"_ZN3art3JNI19DeleteWeakGlobalRefEP7_JNIEnvP8_jobject",
"_ZN3art3JNI12NewGlobalRefEP7_JNIEnvP8_jobject",
"_ZN3art3JNI15DeleteGlobalRefEP7_JNIEnvP8_jobject",
"_ZN3art9JavaVMExt22AddWeakGlobalReferenceEPNS_6ThreadEPNS_6mirror6ObjectE",
"_ZN3art6Thread7CurrentEv",
"_ZN15AndroidPixelRefC1EP7_JNIEnvRK11SkImageInfoPvjP11_jbyteArrayP12SkColorTable",
"_ZNK3art14CompilerDriver31ShouldVerifyClassBasedOnProfileERKNS_7DexFileEt",
// 7.x
"_ZN3art8verifier14MethodVerifier11VerifyClassEPNS_6ThreadEPNS_6mirror5ClassEPNS_17CompilerCallbacksEbNS_11LogSeverityEPNSt3__112basic_stringIcNSA_11char_traitsIcEENSA_9allocatorIcEEEE",
// 6.x
"_ZN3art8verifier14MethodVerifier11VerifyClassEPNS_6ThreadEPNS_6mirror5ClassEbPNSt3__112basic_stringIcNS7_11char_traitsIcEENS7_9allocatorIcEEEE",
// 5.1
"_ZN3art8verifier14MethodVerifier11VerifyClassEPNS_6mirror5ClassEbPNSt3__112basic_stringIcNS5_11char_traitsIcEENS5_9allocatorIcEEEE"
};

// JNI::GetJavaVM
#define INDEX_0 0
typedef u4 (*type_Fun0)(void* env, void** jvm);
u4 Fun0(void* env, void** jvm) {
    type_Fun0 pfnTemp = (type_Fun0)g_a_uOrigFunAddr[INDEX_0];
    return pfnTemp(env, jvm);
}

// object::IdentityHashCode =============
#define INDEX_1 1
typedef u4 (*type_Fun1)(void* This);
u4 Fun1(void* This) {
    if (g_bRawObjAddrSwitch) {
        return (u4)This;
    } else {
        if (lpfnIsNativeBitmapObject) {
            bool bResult = lpfnIsNativeBitmapObject((int*)This);
            if(bResult) {
                return (u4)This;
            }
        }
        type_Fun1 pfnTemp = (type_Fun1)g_a_uOrigFunAddr[INDEX_1];
        return pfnTemp(This);
    }
}

// Object::LargeObjectMapSpaceFree
#define INDEX_2 2
typedef u4 (*type_Fun2)(void* This, void* self, void* obj);
u4 Fun2(void* This, void* self, void* obj) {
    bool bFind = false;
    vector<u4>::iterator iterVector;
    pthread_mutex_lock(&g_mutexNativeBufferPtrVectcor);
    for(iterVector = g_vecNativeBufferPtr.begin();iterVector != g_vecNativeBufferPtr.end();iterVector++)
    {
       if (*iterVector == (u4)obj) {
            bFind = true;
            g_vecNativeBufferPtr.erase(iterVector);
            break;
       }
    }
    pthread_mutex_unlock(&g_mutexNativeBufferPtrVectcor);
    if (bFind) {
        TESTALOGI("VM try to free a mock obj %p", obj);
        int* p_mallocAddr = (int*)obj - 1;
        int mallocAddr = *p_mallocAddr;
        free((void*)mallocAddr);
        TESTALOGI("hook: LargeObjectMapSpaceFree free mallocAddr 0x%x", mallocAddr);

        // remove from map
        map<int ,int>::iterator iterMap;
        bFind = false;
        pthread_mutex_lock(&g_mutexNativeBufferPtrMap);
        for(iterMap = g_mapNativeBufferPtr.begin(); iterMap != g_mapNativeBufferPtr.end(); iterMap++) {
            if ((int)obj == iterMap->second) {
                bFind = true;
                g_mapNativeBufferPtr.erase(iterMap);
                break;
            }
        }
        pthread_mutex_unlock(&g_mutexNativeBufferPtrMap);
        if (bFind) {
            int ref = iterMap->first;
            TESTALOGI("hook: LargeObjectMapSpaceFree remove ref 0x%x mock obj %p", ref, obj);
        }
        return 0; // recycle 0 bytes for ART
    }
    type_Fun2 pfnTemp = (type_Fun2)g_a_uOrigFunAddr[INDEX_2];
    return pfnTemp(This, self, obj);
}

// LargeObjectSpace::Contains
#define INDEX_3 3
typedef bool (*type_Fun3)(void* This, void* obj);
bool Fun3(void* This, void *obj) {
    vector<u4>::iterator iterVector;
    type_Fun3 pfnTemp = (type_Fun3)g_a_uOrigFunAddr[INDEX_3];
    bool bResult = pfnTemp(This, obj);
    bool bFind = false;
    pthread_mutex_lock(&g_mutexNativeBufferPtrVectcor);
    for(iterVector = g_vecNativeBufferPtr.begin();iterVector != g_vecNativeBufferPtr.end();iterVector++)
    {
        if (*iterVector == (u4)obj) {
            bFind = true;
            break;
        }
    }
   pthread_mutex_unlock(&g_mutexNativeBufferPtrVectcor);
   if (bFind && !bResult) {
       TESTALOGI("hook: LargeObjecSpaceContain obj 0x%x change bResult from %d to %d", (int)obj, bResult, true);
       return true;
   }
   return bResult;
}

// VMEXT::AddWeakGlobalRef
#define INDEX_4 4
typedef void* (*type_Fun4)(void* vm, void* thread, void* obj);
void* Fun4(void* vm, void* thread, void* obj) {
    type_Fun4 pfnTemp = (type_Fun4)g_a_uOrigFunAddr[INDEX_4];
    void* pResult = pfnTemp(vm, thread, obj);
    if (g_bGlobalRefLogSwitch) {
        char a_chLog[SZ_ARRAY_LENGTH];
        sprintf(a_chLog, "%s-%08x-%08x-%08x", g_a_szInjectFunName[INDEX_4], (int)thread, (int)obj, (int)pResult);
        pthread_mutex_lock(&g_mutexGlobalRefLogVectcor);
        g_vecGlobalRefLog.push_back(a_chLog);
        pthread_mutex_unlock(&g_mutexGlobalRefLogVectcor);
        //TESTALOGI("hook: %s vm %p thread %p obj %p， return %p", g_a_szInjectFunName[INDEX_4], vm, thread , obj, pResult);
    }
    return pResult;
}

// VMEXT::DeleteWeakGlobalRef
#define INDEX_5 5
typedef void (*type_Fun5)(void* vm, void* thread, void* obj);
void Fun5(void* vm, void* thread, void* obj) {
    type_Fun5 pfnTemp = (type_Fun5)g_a_uOrigFunAddr[INDEX_5];
    pfnTemp(vm, thread, obj);
    return;
}

// VMEXT::AddGlobalRef
#define INDEX_6 6
typedef void* (*type_Fun6)(void* vm, void* thread, void* obj);
void* Fun6(void* vm, void* thread, void* obj) {
    type_Fun6 pfnTemp = (type_Fun6)g_a_uOrigFunAddr[INDEX_6];
    void* pResult = pfnTemp(vm, thread, obj);
    return pResult;
}

// VMEXT::DeleteGlobalRef
#define INDEX_7 7
typedef void (*type_Fun7)(void* vm, void* thread, void* obj);
void Fun7(void* vm, void* thread, void* obj) {
    type_Fun7 pfnTemp = (type_Fun7)g_a_uOrigFunAddr[INDEX_7];
    pfnTemp(vm, thread, obj);
    return;
}

// JNI::NewWeakGlobalRef
#define INDEX_8 8
typedef void* (*type_Fun8)(void* env, void* obj);
void* Fun8(void* env, void* obj) {
    type_Fun8 pfnTemp = (type_Fun8)g_a_uOrigFunAddr[INDEX_8];
    void* pResult = pfnTemp(env, obj);
    return pResult;
}

// JNI::DeleteWeakGlobalRef
#define INDEX_9 9
typedef void (*type_Fun9)(void* env, void* obj);
void Fun9(void* env, void* obj) {
    type_Fun9 pfnTemp = (type_Fun9)g_a_uOrigFunAddr[INDEX_9];
    pfnTemp(env, obj);
    return;
}

// JNI::NewGlobalRef
#define INDEX_10 10
typedef void* (*type_Fun10)(void* env, void* obj);
void* Fun10(void* env, void* obj) {
    type_Fun10 pfnTemp = (type_Fun10)g_a_uOrigFunAddr[INDEX_10];
    void* pResult = pfnTemp(env, obj);
    if (g_bGlobalRefLogSwitch) {
        char a_chLog[SZ_ARRAY_LENGTH];
        sprintf(a_chLog, "%s-%08x-%08x-%08x", "JNI::NewGlobalRef", (int)env, (int)obj, (int)pResult);
        pthread_mutex_lock(&g_mutexGlobalRefLogVectcor);
        g_vecGlobalRefLog.push_back(a_chLog);
        pthread_mutex_unlock(&g_mutexGlobalRefLogVectcor);
        //TESTALOGI("hook: JNI::NewGlobalRef env %p obj %p，return %p", env , obj, pResult);
    }
    if(obj && !pResult) {
        // it is hard to believe we come here ...
        // There is some thing wrong in rendering engine since we can not reproduce it in stressing testing
        TESTALOGI("hook: JNI::NewGlobalRef (GOD-SAVE-ME) env %p obj %p pResult %p", env, obj, pResult);
        bool bFind = false;
        pthread_mutex_lock(&g_mutexNativeBufferPtrMap);
        if(g_mapNativeBufferPtr.find((int)obj) != g_mapNativeBufferPtr.end()){
            bFind = true;
        }
        pthread_mutex_unlock(&g_mutexNativeBufferPtrMap);
        if (bFind) {
            if (lpfnDebugAndAddGlobalRef) {
                void* p_rawObject = (void*)g_mapNativeBufferPtr[(int)obj];
                pResult = lpfnDebugAndAddGlobalRef(env, p_rawObject);
                TESTALOGI("hook: JNI::NewGlobalRef (GOD-SAVE-ME) lpfnDebugAndAddGlobalRef, obj %p pResult %p", p_rawObject, pResult);
            }
        }
    }
    return pResult;
}

// ART::JNI::DeleteGlobalRef
#define INDEX_11 11
typedef void (*type_Fun11)(void* env, void* obj);
void Fun11(void* env, void* obj) {
    type_Fun11 pfnTemp = (type_Fun11)g_a_uOrigFunAddr[INDEX_11];
    pfnTemp(env, obj);
    return ;
}

// VMEXT::AddWeakGlobalRefence 5.x
#define INDEX_12 12
typedef void* (*type_Fun12)(void* vm, void* thread, void* obj);
void* Fun12(void* vm, void* thread, void* obj) {
    type_Fun12 pfnTemp = (type_Fun12)g_a_uOrigFunAddr[INDEX_12];
    void* pResult = pfnTemp(vm, thread, obj);
    return pResult;
}

// Thread::Current
#define INDEX_13 13
typedef void* (*type_Fun13)();
void* Fun13() {
    type_Fun13 pfnTemp = (type_Fun13)g_a_uOrigFunAddr[INDEX_13];
    void* pResult = pfnTemp();
    return pResult;
}

// Android::AndroidPixelRef
#define INDEX_14 14
typedef void (*type_Fun14)(void* This, void* env, int info, void* storage,\
        int rowBytes, void* storageObj, void* ctable);
void Fun14(void* This, void* env, int info, void* storage,\
               int rowBytes, void* storageObj, void* ctable) {
    if (g_bGlobalRefLogSwitch) {
        char a_chLog[SZ_ARRAY_LENGTH];
        sprintf(a_chLog, "%s-%08x-%08x-%08x", g_a_szInjectFunName[14], (int)env, (int)storageObj, (int)storage);
        pthread_mutex_lock(&g_mutexGlobalRefLogVectcor);
        g_vecGlobalRefLog.push_back(a_chLog);
        pthread_mutex_unlock(&g_mutexGlobalRefLogVectcor);
        //TESTALOGI("hook: %s This %p env %p info 0x%x storage %p rowBytes 0x%x storageObj %p ctable %p",\
        //    g_a_szInjectFunName[14], This, env, info, storage, rowBytes, storageObj, ctable);
    }

    type_Fun14 pfnTemp = (type_Fun14)g_a_uOrigFunAddr[INDEX_14];
    pfnTemp(This, env, info, storage, rowBytes, storageObj, ctable);
}

// compiler::shouldverify
#define INDEX_15 15
typedef bool (*type_Fun15)(void* This, void* self, void* kclass);
bool Fun15(void* This, void* self, void* kclass) {
    //type_Fun15 pfnTemp = (type_Fun15)g_a_uOrigFunAddr[INDEX_15];
    //pfnTemp(This, self, kclass);
    return false;
}

#define INDEX_16 16
typedef int (*type_Fun16)(void* This, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6);
int Fun16(void* This, void* p1, void* p2, void* p3, void* p4, void* p5, void* p6) {
    asm(
        "nop \n\n"
    );
    return 0;
}

#define INDEX_17 17
typedef int (*type_Fun17)(void* This, void* p1, void* p2, void* p3, void* p4);
int Fun17(void* This, void* p1, void* p2, void* p3, void* p4) {
    asm(
        "nop \n\n"
    );
    return 0;
}

#define INDEX_18 18
typedef int (*type_Fun18)(void* This, void* p1, void* p2, void* p3);
int Fun18(void* This, void* p1, void* p2, void* p3) {
    asm(
        "nop \n\n"
    );
    return 0;
}

u4 g_a_uInjectFunAddr[INJECT_FUN_NUM] = {
(u4)Fun0,
(u4)Fun1,
(u4)Fun2,
(u4)Fun3,
(u4)Fun4,
(u4)Fun5,
(u4)Fun6,
(u4)Fun7,
(u4)Fun8,
(u4)Fun9,
(u4)Fun10,
(u4)Fun11,
(u4)Fun12,
(u4)Fun13,
(u4)Fun14,
(u4)Fun15,
(u4)Fun16,
(u4)Fun17,
(u4)Fun18};

void LaunchBinaryInject(int sdkvesion) {
    Cydia::elfHookFunctionEx(SO_LIBART, &(g_a_szSymbolName[INDEX_0]),
            (void**)&(g_a_uInjectFunAddr[INDEX_0]), (void**)&(g_a_uOrigFunAddr[INDEX_0]), 14);

    Cydia::elfHookFunctionEx(SO_LIBANDROIDRUNTIME, &(g_a_szSymbolName[INDEX_14]),
        (void**)&(g_a_uInjectFunAddr[INDEX_14]), (void**)&(g_a_uOrigFunAddr[INDEX_14]), 1);

    //Cydia::elfHookFunctionEx(SO_LIBCOMPILER, &(g_a_szSymbolName[INDEX_15]),
    //    (void**)&(g_a_uInjectFunAddr[INDEX_15]), (void**)&(g_a_uOrigFunAddr[INDEX_15]), 1);

    Cydia::elfHookFunctionEx(SO_LIBART, &(g_a_szSymbolName[INDEX_16]),
        (void**)&(g_a_uInjectFunAddr[INDEX_16]), (void**)&(g_a_uOrigFunAddr[INDEX_16]), 3);
}

void EnableCheckSupport(void *addrFunction) {
	g_bRawObjAddrSwitch = true;
    g_bGlobalRefLogSwitch = true;
    TESTALOGI("enter check support mode");
}

void DisableCheckSupport(void *addrFunction) {
    g_bRawObjAddrSwitch = false;
    g_bGlobalRefLogSwitch = false;
    TESTALOGI("exit check support mode");
}

bool ParseGlobalRefLog(int target, void** p1, void** p2, void** p3, const char* tag) {
    const char* szLine;
    char a_chTarget[SZ_ARRAY_LENGTH];
    sprintf(a_chTarget, "%08x", target);
    vector<string>::iterator iterVector;
    bool bFind = false;;
    pthread_mutex_lock(&g_mutexGlobalRefLogVectcor);
    for(iterVector = g_vecGlobalRefLog.begin();iterVector != g_vecGlobalRefLog.end();iterVector++)
    {
        szLine = (*iterVector).c_str();
        if (strstr(szLine, tag) && strstr(szLine, a_chTarget)) {
            bFind = true;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutexGlobalRefLogVectcor);
    if (bFind) {
        const char* szDataField = strstr(szLine, "-");
        if (szDataField) {
            int iResult = sscanf(szDataField, "-%08x-%08x-%08x", (u4 *)p1, (u4 *)p2, (u4 *)p3);
            if (iResult == 3) {
                return true;
            }
        }
    }
    return false;
}

/*
 * get global ref from log
 */
bool GetGlobalRefFromLog(int target, int offset, void** ref) {
    char a_chTarget[SZ_ARRAY_LENGTH];
    void *p1, *p2, *p3;

#ifdef GLOBALREF_DEBUG
    vector<string>::iterator iterVector;
    pthread_mutex_lock(&g_mutexGlobalRefLogVectcor);
    for(iterVector = g_vecGlobalRefLog.begin();iterVector != g_vecGlobalRefLog.end();iterVector++)
    {
        const char* szLine = (*iterVector).c_str();
        TESTALOGI("strLine %s", szLine);
    }
    pthread_mutex_unlock(&g_mutexGlobalRefLogVectcor);
#endif
    bool bResult = ParseGlobalRefLog(target, &p1, &p2, &p3, g_a_szInjectFunName[INDEX_4]);
    if (bResult) {
        // for 6.x~7.x
        // system uses VMEXT::AddWeakGlobalRef to create the weak global ref
        // then leverage it to create global ref in pinPixelRef()
        TESTALOGI("%s thread %p obj %p, pResult %p", g_a_szInjectFunName[INDEX_4], p1, p2, p3);
        *ref = p3;
        return true;
    } else {
        // for 5.x
        // system uses JNI::NewGlobalRef to create the global ref by storageObj (local ref)
        // AndroidPixRef constructor have parameters storageObj and storage
        int iTargetAddOffset = target+offset;
        bResult = ParseGlobalRefLog(iTargetAddOffset, &p1, &p2, &p3, g_a_szInjectFunName[14]); // find storage first
        if(bResult) {
            TESTALOGI("%s env %p storageObj %p storage %p", g_a_szInjectFunName[14], p1, p2, p3);
            int iLocaRef = (int)p2;
            bResult = ParseGlobalRefLog(iLocaRef, &p1, &p2, &p3, g_a_szInjectFunName[10]); // then find ref
            if (bResult) {
                TESTALOGI("%s env %p obj %p pResult, %p", g_a_szInjectFunName[10], p1, p2, p3);
                *ref = p3;
                return true;
            }
        }
    }
    return false;
}

/*
 * get Thread ptr from log
 */
bool GetThreadFromLog(int target, void** ptrThread) {
    void *p1, *p2, *p3;
    bool bResult = ParseGlobalRefLog(target, &p1, &p2, &p3, g_a_szInjectFunName[INDEX_4]);
    if (bResult) {
        // search for 6.x~7.x
        // system uses VMEXT::AddWeakGlobalRef to create the global ref
        // then leverage it to create global ref in pinPixelRef()
        TESTALOGI("%s thread %p obj %p, pResult %p", g_a_szInjectFunName[INDEX_4], p1, p2, p3);
        *ptrThread = p1;
        return true;
    }
    return false;
}

/*
 * regist callback function
 */
bool RegistCallback(void* callback, const char* fnName) {
    if (strcmp(g_szIsNativeBitmapObject, fnName) == 0) {
        lpfnIsNativeBitmapObject = (LPFNISISNATIVEBITMAPOBJECT)callback;
    }
    if (strcmp(g_szAddGlobalRef, fnName) == 0) {
        lpfnDebugAndAddGlobalRef = (FLPFNDEBUGANDADDGLOBALREFF)callback;
    }
    if (strcmp(g_szLOGI, fnName) == 0) {
        lpfnLOGI = (FLPFNLOGI)callback;
    }
    if (strcmp(g_szLOGD, fnName) == 0) {
        lpfnLOGD = (FLPFNLOGD)callback;
    }
    if (strcmp(g_szLOGE, fnName) == 0) {
        lpfnLOGE = (FLPFNLOGE)callback;
    }
    return true;
}

/*
 * insert NativeBuffer ptr to Vector
 */
void InsertNativeBufferPtrVector(void* ptr) {
    pthread_mutex_lock(&g_mutexNativeBufferPtrVectcor);
    g_vecNativeBufferPtr.push_back((u_int32_t) ptr);
    pthread_mutex_unlock(&g_mutexNativeBufferPtrVectcor);
}

/*
 * insert NativeBuffer ptr-ref to Map
 */
void InsertNativeBufferPtrMap(void* ptr, void* ref) {
    pthread_mutex_lock(&g_mutexNativeBufferPtrMap);
    g_mapNativeBufferPtr[(int)ptr] = (int)ref;
    pthread_mutex_unlock(&g_mutexNativeBufferPtrMap);
}
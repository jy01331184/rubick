#ifndef ____FastLoadDex__
#define ____FastLoadDex__

#ifdef __cplusplus
extern "C"  //C++
{
#endif
    // enable/disable raw object addr and ref log
    void EnableCheckSupport(void *env);
    void DisableCheckSupport(void *env);
    // launch inject
    void LaunchBinaryInject(int sdkvesion);
    // regist callback
    bool RegistCallback(void* callbak, const char* funName);
    // insert NativeBuffer ptr to vector
    void InsertNativeBufferPtrVector(void* ptr);
    // insert NativeBuffer ptr-ref to map
    void InsertNativeBufferPtrMap(void* ptr, void* ref);
    // parse thread ptr which locates in evn
    bool GetThreadFromLog(int jbytearrayPtr, void** result);
    // parse global ref
    bool GetGlobalRefFromLog(int targetAddr, int offset, void** result);
#ifdef __cplusplus
}
#endif

#endif /* defined(____FastLoadDex__) */

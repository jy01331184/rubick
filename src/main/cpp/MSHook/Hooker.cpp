#include <unistd.h>
#include "Hooker.h"
#include "ARM.h"
#include "Thumb.h"
#include "x86.h"

_extern void Cydia::MSHookFunction(void *symbol, void *replace, void **result, const char* name) {

    SubstrateProcessRef process = NULL;
#if defined(__arm__) || defined(__thumb__)
    if( (unsigned int)symbol % 4 == 0) {
        ARM::SubstrateHookFunctionARM(process, symbol, replace, result);
    }else{
        Thumb::SubstrateHookFunctionThumb(process, \
		reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(symbol) & ~0x1), replace, result);
    }
#endif


#if defined(__i386__) || defined(__x86_64__)
    x86::SubstrateHookFunctionx86(process, symbol, replace, result);
#endif
    return;
}

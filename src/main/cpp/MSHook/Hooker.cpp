#include <unistd.h>
#include "Hooker.h"
#include "../util.h"
#include "ARM.h"
#include "Thumb.h"
#include "x86.h"

extern volatile FLPFNLOGI lpfnLOGI;
extern volatile FLPFNLOGD lpfnLOGD;

//#define TESTALOGI(fmt,...) if(lpfnLOGI){lpfnLOGI(fmt, ##__VA_ARGS__);}
//#define TESTALOGD(fmt,...) if(lpfnLOGD){lpfnLOGD(fmt, ##__VA_ARGS__);}

_extern void Cydia::MSHookFunction(void *symbol, void *replace, void **result, const char* name) {

    SubstrateProcessRef process = NULL;
#if defined(__arm__) || defined(__thumb__)
    if( (unsigned int)symbol % 4 == 0) {
	    TESTALOGI("ARM code inject, name %s symbol %p, 0x%x", name, symbol, (unsigned int)symbol%4);
        ARM::SubstrateHookFunctionARM(process, symbol, replace, result);
    }else{
	    TESTALOGI("Thumb code inject, name %s symbol %p 0x%x", name, symbol, (unsigned int)symbol%4);
        Thumb::SubstrateHookFunctionThumb(process, \
		reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(symbol) & ~0x1), replace, result);
    }
#endif


#if defined(__i386__) || defined(__x86_64__)
    x86::SubstrateHookFunctionx86(process, symbol, replace, result);
#endif
    return;
}

_extern void Cydia::elfHookFunctionEx(const char* soname, const char* symbol[], void* replace_func[], void* old_func[], int count) {
    unsigned long addrArray[128];
    unsigned long sizeArray[128];
    int resultArray[128];

    if (count >= 128) {
        TESTALOGD(" too large inject request");
        return;
    }

    int r = getSymbolAddrAndSize(getpid(), symbol, soname, addrArray, sizeArray, resultArray, count);
    if (r < 0) {
        TESTALOGD("invalid symbol for %s", soname);
        return;
    }

    char str[128];
    sprintf(str, "/system/lib/%s", soname);
    int offset = getFuctionOffsetBase(str);
    for (int i = 0; i < count; i++) {
        if (resultArray[i] == 0) {
            Cydia::MSHookFunction((char*)addrArray[i]-offset, replace_func[i], &(old_func[i]), symbol[i]);
        }
    }
}

#ifndef HOOKER_H_
#define HOOKER_H_

#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>

#include "Debug.h"
#include "Log.h"
#include "PosixMemory.h"
#include "CydiaSubstrate.h"

namespace Cydia{
    _extern void MSHookFunction(void *symbol, void *replace, void **result, const char* name);

}
#endif /* HOOKER_H_ */
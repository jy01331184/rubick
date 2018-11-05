//
// Created by wangzuo.wz on 16/9/20.
//
//  Binary Instrumentation for Suppressing GC Activity for Android Dalvik
//  WangZuo <wangzuo.wz@ant-financial.com>
//

#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <android/log.h>
#include<stdlib.h>
#include "util.h"

int getFuctionOffsetBase(const char* libName) {
    int fd;
    fd = open(libName, O_RDONLY);
    if (-1 == fd) {
        TESTALOGE("getFuctionOffsetBase() open %s error\n", libName);
        return 0;
    }

    Elf32_Ehdr ehdr;
    read(fd, &ehdr, sizeof(Elf32_Ehdr));

    unsigned long shdr_addr = ehdr.e_shoff;
    int shnum = ehdr.e_shnum;
    int shent_size = ehdr.e_shentsize;
    unsigned long stridx = ehdr.e_shstrndx;

    Elf32_Shdr shdr;

    // load string table
    lseek(fd, shdr_addr + stridx * shent_size, SEEK_SET);
    read(fd, &shdr, shent_size);
    char * string_table = (char *)malloc(shdr.sh_size);
    if (!string_table) {
        TESTALOGE("getFuctionOffsetBase() malloc fail \n", model);
        return 0;
    }
    lseek(fd, shdr.sh_offset, SEEK_SET);
    read(fd, string_table, shdr.sh_size);

    // reset
    lseek(fd, shdr_addr, SEEK_SET);

    // In executable case， some entry_address and entry_offset does not match
    uint32_t first_entry_addr = 0;
    uint32_t first_entry_offset = 0;
    int j;
    for (j = 0; j < shnum; j++) {
        read(fd, &shdr, shent_size);
        int name_idx = shdr.sh_name;
        if (*((char*)&(string_table[name_idx])) != 0) {
            first_entry_addr = shdr.sh_addr;
            first_entry_offset = shdr.sh_offset;
            TESTALOGE("getFuctionOffsetBase %s \n", &(string_table[name_idx]));
            TESTALOGE("getFuctionOffsetBase first_entry_addr = 0x%x  first_entry_offset = 0x%x \n", first_entry_addr, first_entry_offset);
            break;
        }
    }

    return first_entry_addr - first_entry_offset;
}

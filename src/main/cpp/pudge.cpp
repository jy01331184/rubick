//
// Created by tianyang on 2018/11/5.
//

#include <sys/types.h>
#include <malloc.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "pudge.h"
#include "MSHook/Hooker.h"
#define MAX_NAME_LEN 256
#define MEMORY_ONLY  "[memory]"
#define MMARRAYSIZE 1024*128
#define RAWLINES 1024*1024*8

typedef struct SymbolTab *P_SymbolTab;
struct SymbolList {
    Elf32_Sym *sym;       /* symbols */
    char *str;            /* symbol strings */
    unsigned num;         /* number of symbols */
};
struct SymbolTab {
    struct SymbolList *st;    /* "static" symbols */
    struct SymbolList *dyn;   /* dynamic symbols */
};
struct MemoryMap {
    char name[MAX_NAME_LEN];
    unsigned long start, end;
};

int my_pread(int fd, void *buf, size_t count, off_t offset) {
    lseek(fd, offset, SEEK_SET);
    return read(fd, buf, count);
}

int lookupInternel(struct SymbolList *sl, unsigned char type, const char *name, unsigned int *val,
                   unsigned int *size) {
    Elf32_Sym *p;
    int len;
    int i;

    len = strlen(name);
    for (i = 0, p = sl->sym; i < sl->num; i++, p++) {
        if (!strncmp(sl->str + p->st_name, name, len) && *(sl->str + p->st_name + len) == 0
            && ELF32_ST_TYPE(p->st_info) == type) {
            //if (p->st_value != 0) {
            *val = p->st_value;
            *size = p->st_size;
            return 1;
            //}
        }
    }
    return 0;
}

struct SymbolList *getSymbolList(int fd, Elf32_Shdr *symh, Elf32_Shdr *strh) {
    struct SymbolList *sl, *ret;
    int rv;

    ret = NULL;
    sl = (struct SymbolList *) malloc(sizeof(struct SymbolList));
    sl->str = NULL;
    sl->sym = NULL;

    /* sanity */
    if (symh->sh_size % sizeof(Elf32_Sym)) {
        //printf("elf_error\n");
        goto out;
    }

    /* symbol table */
    sl->num = symh->sh_size / sizeof(Elf32_Sym);
    sl->sym = (Elf32_Sym *) malloc(symh->sh_size);
    rv = my_pread(fd, sl->sym, symh->sh_size, symh->sh_offset);
    if (0 > rv) {
        //perror("read");
        goto out;
    }
    if (rv != symh->sh_size) {
        //printf("elf error\n");
        goto out;
    }

    /* string table */
    sl->str = (char *) malloc(strh->sh_size);
    rv = my_pread(fd, sl->str, strh->sh_size, strh->sh_offset);
    if (0 > rv) {
        //perror("read");
        goto out;
    }
    if (rv != strh->sh_size) {
        //printf("elf error");
        goto out;
    }

    ret = sl;
    out:
    return ret;
}

int doLoad(int fd, P_SymbolTab symtab) {
    int rv;
    size_t size;
    Elf32_Ehdr ehdr;
    Elf32_Shdr *shdr = NULL, *p;
    Elf32_Shdr *dynsymh, *dynstrh;
    Elf32_Shdr *symh, *strh;
    char *shstrtab = NULL;
    int i;
    int ret = -1;

    /* elf header */
    rv = read(fd, &ehdr, sizeof(ehdr));
    if (0 > rv) {
        goto out;
    }
    if (rv != sizeof(ehdr)) {
        goto out;
    }
    if (strncmp(ELFMAG, (char *) ehdr.e_ident, SELFMAG)) { /* sanity */
        goto out;
    }
    if (sizeof(Elf32_Shdr) != ehdr.e_shentsize) { /* sanity */
        goto out;
    }

    /* section header table */
    size = ehdr.e_shentsize * ehdr.e_shnum;
    shdr = (Elf32_Shdr *) malloc(size);
    rv = my_pread(fd, shdr, size, ehdr.e_shoff);
    if (0 > rv) {
        goto out;
    }
    if (rv != size) {
        goto out;
    }

    /* section header string table */
    size = shdr[ehdr.e_shstrndx].sh_size;
    shstrtab = (char *) malloc(size);
    rv = my_pread(fd, shstrtab, size, shdr[ehdr.e_shstrndx].sh_offset);
    if (0 > rv) {
        goto out;
    }
    if (rv != size) {
        goto out;
    }

    /* symbol table headers */
    symh = dynsymh = NULL;
    strh = dynstrh = NULL;
    for (i = 0, p = shdr; i < ehdr.e_shnum; i++, p++)
        if (SHT_SYMTAB == p->sh_type) {
            if (symh) {
                LOGD("too many symbol tables\n");
                goto out;
            }
            symh = p;
        } else if (SHT_DYNSYM == p->sh_type) {
            if (dynsymh) {
                LOGD("too many symbol tables\n");
                goto out;
            }
            dynsymh = p;
        } else if (SHT_STRTAB == p->sh_type
                   && !strncmp(shstrtab + p->sh_name, ".strtab", 7)) {
            if (strh) {
                LOGD("too many string tables\n");
                goto out;
            }
            strh = p;
        } else if (SHT_STRTAB == p->sh_type
                   && !strncmp(shstrtab + p->sh_name, ".dynstr", 7)) {
            if (dynstrh) {
                LOGD("too many string tables\n");
                goto out;
            }
            dynstrh = p;
        }
    /* sanity checks */
    if ((!dynsymh && dynstrh) || (dynsymh && !dynstrh)) {
        LOGD("bad dynamic symbol table\n");
        goto out;
    }
    if ((!symh && strh) || (symh && !strh)) {
        LOGD("bad symbol table\n");
        goto out;
    }
    if (!dynsymh && !symh) {
        LOGD("no symbol table\n");
        goto out;
    }

    /* symbol tables */
    if (dynsymh)
        symtab->dyn = getSymbolList(fd, dynsymh, dynstrh);
    if (symh)
        symtab->st = getSymbolList(fd, symh, strh);
    ret = 0;
    out:
    free(shstrtab);
    free(shdr);
    return ret;
}

/* 读取 /proc/pid/maps 收集所有 so start-end */
int loadMemMap(pid_t pid, struct MemoryMap *mm, int *nmmp) {
    char name[MAX_NAME_LEN];
    char *p;
    unsigned long start, end;
    struct MemoryMap *m;
    int nmm = 0;
    int fd, rv;
    int i;

    char *raw = (char *) malloc(RAWLINES);
    if (!raw) {
        LOGD("Can't alloc buffer raw, size %d \n", RAWLINES);
        return -1;
    }

    sprintf(raw, "/proc/%d/maps", pid);
    fd = open(raw, O_RDONLY);
    if (0 > fd) {
        LOGD("Can't open %s for reading\n", raw);
        free(raw);
        return -1;
    }

    /* Zero to ensure data is null terminated */
    memset(raw, 0, RAWLINES);

    p = raw;
    while (1) {
        rv = read(fd, p, RAWLINES - (p - raw));
        if (0 > rv) {
            LOGD("read failure");
            free(raw);
            return -1;
        }
        if (0 == rv)
            break;
        p += rv;
        if (p - raw >= RAWLINES) {
            LOGD("Too many memory mapping\n");
            free(raw);
            return -1;
        }
    }
    close(fd);

    p = strtok(raw, "\n");
    m = mm;
    while (p) {
        /* parse current map line */
        rv = sscanf(p, "%08lx-%08lx %*s %*s %*s %*s %s\n",
                    &start, &end, name);

        p = strtok(NULL, "\n");

        if (rv == 2) {
            m = &mm[nmm++];
            m->start = start;
            m->end = end;
            strcpy(m->name, MEMORY_ONLY);
            continue;
        }

        /* search backward for other mapping with same name */
        for (i = nmm - 1; i >= 0; i--) {
            m = &mm[i];
            if (!strcmp(m->name, name))
                break;
        }

        if (i >= 0) {
            if (start < m->start)
                m->start = start;
            if (end > m->end)
                m->end = end;
        } else {
            /* new entry */
            m = &mm[nmm++];
            m->start = start;
            m->end = end;
            strcpy(m->name, name);
        }

        if (nmm >= MMARRAYSIZE) {
            LOGD(" nm >= MMARRAYSIZE");
            break;
        }
    }

    *nmmp = nmm;
    free(raw);
    return 0;
}

/* 根据 soname 匹配对应的 start-end 写入 MemoryMap*/
int getTargetLibAddr(const char *soname, char *name, int len, unsigned long *start,
                     struct MemoryMap *mm, int count) {
    int i;
    struct MemoryMap *m;
    char *p;
    for (i = 0, m = mm; i < count; i++, m++) {
        if (!strcmp(m->name, MEMORY_ONLY))
            continue;
        p = strrchr(m->name, '/');
        if (!p)
            continue;
        p++;
        if (strncmp(soname, p, strlen(soname)))
            continue;
        p += strlen(soname);

        /* here comes our crude test -> 'libc.so' or 'libc-[0-9]' */
        if (!strncmp("so", p, 2) || 1) // || (p[0] == '-' && isdigit(p[1])))
            break;
    }
    if (i >= count)
        /* not found */
        return 0;

    *start = m->start;
    strncpy(name, m->name, len);
    if (strlen(m->name) >= len)
        name[len - 1] = '\0';

    mprotect((void *) m->start, m->end - m->start, PROT_READ | PROT_WRITE | PROT_EXEC);
    return 1;
}
/* 根据so 的 path 读取解析 P_SymbolTab 包含静态段 动态段 */
P_SymbolTab loadSymbolTab(char *filename) {
    int fd;
    P_SymbolTab symtab;

    symtab = (P_SymbolTab) malloc(sizeof(*symtab));
    memset(symtab, 0, sizeof(*symtab));
    LOGD("open file %s",filename);
    fd = open(filename, O_RDONLY);
    if (0 > fd) {
        LOGD("%s open\n", __func__);
        free(symtab);
        return NULL;
    }
    if (0 > doLoad(fd, symtab)) {
        LOGD("Error ELF parsing %s\n", filename);
        free(symtab);
        symtab = NULL;
    }
    close(fd);
    return symtab;
}
/* 根据P_SymbolTab 找到对应的符号 */
int lookupSymbol(P_SymbolTab s, unsigned char type, const char *name, unsigned int *val,
                 unsigned int *size) {
    if (s->dyn && lookupInternel(s->dyn, type, name, val, size))
        return 1;
    if (s->st && lookupInternel(s->st, type, name, val, size))
        return 1;
    return 0;
}

int getSymbolOffset(const char *libName) {
    int fd;
    fd = open(libName, O_RDONLY);
    if (-1 == fd) {
        LOGD(" open %s error", libName);
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
    char *string_table = (char *) malloc(shdr.sh_size);
    if (!string_table) {
        LOGD("malloc fail");
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
        if (*((char *) &(string_table[name_idx])) != 0) {
            first_entry_addr = shdr.sh_addr;
            first_entry_offset = shdr.sh_offset;
            //LOGD("%s %s \n", libName, &(string_table[name_idx]));
            //LOGD("%s first_entry_addr = 0x%x  first_entry_offset = 0x%x \n", libName,first_entry_addr, first_entry_offset);
            break;
        }
    }
    return first_entry_addr - first_entry_offset;
}

bool available = true;
struct MemoryMap *p_array_memmap = 0;
int memmapCount = 0;

int pudge::hookFunction(char *libSo, char *targetSymbol, void *newFunc, void **oldFunc) {
    if(!available){
        return 0;
    }
    if(!p_array_memmap){
        p_array_memmap = (struct MemoryMap *) malloc(MMARRAYSIZE * sizeof(struct MemoryMap));
        if (loadMemMap(getpid(), p_array_memmap, &memmapCount)) {
            available = false;
            return 0;
        }
        LOGD("totalCount = %d", memmapCount);
    }

    char libName[1024];
    unsigned long start;
    if (!getTargetLibAddr(libSo, libName, sizeof(libName), &start, p_array_memmap, memmapCount)) {
        return 0;
    }
    LOGD("find %s %lx", libName, start);

    P_SymbolTab p_symbol = loadSymbolTab(libName);
    if (!p_symbol) {
        return 0;
    }

    unsigned int addr = 0;
    unsigned int size = 0;
    int result = lookupSymbol(p_symbol,STT_FUNC,targetSymbol,&addr,&size);
    if(!result){
        return 0;
    }

    if(size >= 16){
        addr = addr + start;
    }

    char str[128];
    sprintf(str, "/system/lib/%s", libSo);
    int offset = getSymbolOffset(str);

    LOGD("result addr:0x%lx size:%d offset:%d ",addr,size,offset);
    if(addr  && size > 0){
        Cydia::MSHookFunction((char*)addr-offset, newFunc,(oldFunc), targetSymbol);
        return 1;
    }

    return 0;
}

int pudge:: search(int addr, int target, int maxSearch) {
    int *p_addr = reinterpret_cast<int *>(addr);

    for (int i = 0; i < maxSearch; ++i) {

        int tempValue = *(p_addr + i);
        if (tempValue == target) {
            return i;
        }
    }
    return -1;
}
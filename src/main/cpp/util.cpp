/*
 * Elf parsing code taken from: hijack.c (for x86)
 * by Victor Zandy <zandy[at]cs.wisc.edu>
 *
 * Elf parsing code slightly modified for this project
 * (c) Collin Mulliner <collin[at]mulliner.org>
 *
 * License: LGPL v2.1
 *  
 * Termios code taken from glibc with slight modifications for this project
 * 
 */
#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <elf.h>
#include <unistd.h>
#include <errno.h>       
#include <sys/mman.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <android/log.h>
#include <atomic>

#include "util.h"

using namespace std;

/* memory map for libraries */
#define MAX_NAME_LEN 256
#define MEMORY_ONLY  "[memory]"

/*
int offsetLibandroidruntime = 0;
int offsetLibart = 0;

std::atomic<int> g_atomicProcessLibRuntime = ATOMIC_VAR_INIT(0);
std::atomic<int> g_atomicProcessLibArt = ATOMIC_VAR_INIT(0);
*/

extern volatile FLPFNLOGI lpfnLOGI;
extern volatile FLPFNLOGD lpfnLOGD;

//#define TESTALOGI(fmt,...) if(lpfnLOGI){lpfnLOGI(fmt, ##__VA_ARGS__);}
//#define TESTALOGD(fmt,...) if(lpfnLOGD){lpfnLOGD(fmt, ##__VA_ARGS__);}

#define MMARRAYSIZE 1024*128
#define RAWLINES 1024*1024*8

extern void* get_module_base(pid_t pid,  const char* module_name);

struct mm {
	char name[MAX_NAME_LEN];
	unsigned long start, end;
};

typedef struct symtab *symtab_t;
struct symlist {
	Elf32_Sym *sym;       /* symbols */
	char *str;            /* symbol strings */
	unsigned num;         /* number of symbols */
};
struct symtab {
	struct symlist *st;    /* "static" symbols */
	struct symlist *dyn;   /* dynamic symbols */
};

static void* xmalloc(size_t size)
{
	void *p;
	p = malloc(size);
	if (!p) {
		printf("Out of memory\n");
		exit(1);
	}
	return p;
}

static int my_pread(int fd, void *buf, size_t count, off_t offset)
{
	lseek(fd, offset, SEEK_SET);
	return read(fd, buf, count);
}

static struct symlist* get_syms(int fd, Elf32_Shdr *symh, Elf32_Shdr *strh)
{
	struct symlist *sl, *ret;
	int rv;

	ret = NULL;
	sl = (struct symlist *) xmalloc(sizeof(struct symlist));
	sl->str = NULL;
	sl->sym = NULL;

	/* sanity */
	if (symh->sh_size % sizeof(Elf32_Sym)) { 
		//printf("elf_error\n");
		goto out;
	}

	/* symbol table */
	sl->num = symh->sh_size / sizeof(Elf32_Sym);
	sl->sym = (Elf32_Sym *) xmalloc(symh->sh_size);
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
	sl->str = (char *) xmalloc(strh->sh_size);
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

static int do_load(int fd, symtab_t symtab)
{
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
	if (strncmp(ELFMAG, (char *)ehdr.e_ident, SELFMAG)) { /* sanity */
		goto out;
	}
	if (sizeof(Elf32_Shdr) != ehdr.e_shentsize) { /* sanity */
		goto out;
	}

	/* section header table */
	size = ehdr.e_shentsize * ehdr.e_shnum;
	shdr = (Elf32_Shdr *) xmalloc(size);
	rv = my_pread(fd, shdr, size, ehdr.e_shoff);
	if (0 > rv) {
		goto out;
	}
	if (rv != size) {
		goto out;
	}
	
	/* section header string table */
	size = shdr[ehdr.e_shstrndx].sh_size;
	shstrtab = (char *) xmalloc(size);
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
				TESTALOGI("too many symbol tables\n");
				goto out;
			}
			symh = p;
		} else if (SHT_DYNSYM == p->sh_type) {
			if (dynsymh) {
				TESTALOGI("too many symbol tables\n");
				goto out;
			}
			dynsymh = p;
		} else if (SHT_STRTAB == p->sh_type
			   && !strncmp(shstrtab+p->sh_name, ".strtab", 7)) {
			if (strh) {
				TESTALOGI("too many string tables\n");
				goto out;
			}
			strh = p;
		} else if (SHT_STRTAB == p->sh_type
			   && !strncmp(shstrtab+p->sh_name, ".dynstr", 7)) {
			if (dynstrh) {
				TESTALOGI("too many string tables\n");
				goto out;
			}
			dynstrh = p;
		}
	/* sanity checks */
	if ((!dynsymh && dynstrh) || (dynsymh && !dynstrh)) {
		TESTALOGI("bad dynamic symbol table\n");
		goto out;
	}
	if ((!symh && strh) || (symh && !strh)) {
		TESTALOGI("bad symbol table\n");
		goto out;
	}
	if (!dynsymh && !symh) {
		TESTALOGI("no symbol table\n");
		goto out;
	}

	/* symbol tables */
	if (dynsymh)
		symtab->dyn = get_syms(fd, dynsymh, dynstrh);
	if (symh)
		symtab->st = get_syms(fd, symh, strh);
	ret = 0;
out:
	free(shstrtab);
	free(shdr);
	return ret;
}

static symtab_t load_symtab(char *filename)
{
	int fd;
	symtab_t symtab;

	symtab = (symtab_t) xmalloc(sizeof(*symtab));
	memset(symtab, 0, sizeof(*symtab));

	fd = open(filename, O_RDONLY);
	if (0 > fd) {
		TESTALOGI("%s open\n", __func__);
		free(symtab);
		return NULL;
	}
	if (0 > do_load(fd, symtab)) {
		TESTALOGI("Error ELF parsing %s\n", filename);
		free(symtab);
		symtab = NULL;
	}
	close(fd);
	return symtab;
}

static int load_memmap(pid_t pid, struct mm *mm, int *nmmp)
{
	char name[MAX_NAME_LEN];
	char *p;
	unsigned long start, end;
	struct mm *m;
	int nmm = 0;
	int fd, rv;
	int i;

	char* raw = (char*)malloc(RAWLINES);
	if (!raw) {
        TESTALOGI("Can't alloc buffer raw, size %d \n", RAWLINES);
        return -1;
	}

	sprintf(raw, "/proc/%d/maps", pid);
	fd = open(raw, O_RDONLY);
	if (0 > fd) {
		TESTALOGI("Can't open %s for reading\n", raw);
		free(raw);
		return -1;
	}

	/* Zero to ensure data is null terminated */
	memset(raw, 0, RAWLINES);

	p = raw;
	while (1) {
		rv = read(fd, p, RAWLINES-(p-raw));
		if (0 > rv) {
			TESTALOGI("read failure");
			free(raw);
			return -1;
		}
		if (0 == rv)
			break;
		p += rv;
		if (p-raw >= RAWLINES) {
			TESTALOGI("Too many memory mapping\n");
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
		for (i = nmm-1; i >= 0; i--) {
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
		    TESTALOGI(" nm >= MMARRAYSIZE");
		    break;
		}
	}

	*nmmp = nmm;
	free(raw);
	return 0;
}

/* Find libc in MM, storing no more than LEN-1 chars of
   its name in NAME and set START to its starting
   address.  If libc cannot be found return -1 and
   leave NAME and START untouched.  Otherwise return 0
   and null-terminated NAME. */
static int getLibAddr(const char *soname, char *name, int len, unsigned long *start, struct mm *mm, int nmm)
{
	int i;
	struct mm *m;
	char *p;
	for (i = 0, m = mm; i < nmm; i++, m++) {
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
	if (i >= nmm)
		/* not found */
		return -1;

	//TESTALOGI("getLibAddr(): m->name %s m->start %x m->end %x",  m->name, m->start, m->end );

	*start = m->start;
	strncpy(name, m->name, len);
	if (strlen(m->name) >= len)
		name[len-1] = '\0';
		
	mprotect((void*)m->start, m->end - m->start, PROT_READ|PROT_WRITE|PROT_EXEC);
	return 0;
}

static int lookup2(struct symlist *sl, unsigned char type,
	const char *name, unsigned long *val, unsigned long *size)
{
	Elf32_Sym *p;
	int len;
	int i;

	len = strlen(name);
	for (i = 0, p = sl->sym; i < sl->num; i++, p++) {
		if (!strncmp(sl->str+p->st_name, name, len) && *(sl->str+p->st_name+len) == 0
		    && ELF32_ST_TYPE(p->st_info) == type) {
			//if (p->st_value != 0) {
			*val = p->st_value;
			*size = p->st_size;
			return 0;
			//}
		}
	}
	return -1;
}

static int lookup_sym(symtab_t s, unsigned char type,
	   const char *name, unsigned long *val, unsigned long *size)
{
	if (s->dyn && !lookup2(s->dyn, type, name, val, size))
		return 0;
	if (s->st && !lookup2(s->st, type, name, val, size))
		return 0;
	return -1;
}

static int lookup_func_sym(symtab_t s, const char *name, unsigned long *val, unsigned long *size)
{
	return lookup_sym(s, STT_FUNC, name, val,size);
}

static void freeMMStructure(symtab_t s) {
    if(s->st) {
        symlist* sl = s->st;
        if (sl->sym) {
            free(sl->sym);
        }
        if (sl->str) {
            free(sl->str);
        }
        free(s->st);
    }
    if(s->dyn) {
        symlist* sl = s->dyn;
        if (sl->sym) {
            free(sl->sym);
        }
        if (sl->str) {
            free(sl->str);
        }
        free(s->dyn);
    }
    free(s);
}

int getSymbolAddrAndSize(pid_t pid, const char* symbol[], const char *soname, unsigned long addrArray[], \
                        unsigned long sizeArray[], int result[], int count)
{
	unsigned long libaddr;
	int nmm;
	char lib[1024];
	symtab_t s;

    struct mm* p_a_mm = (struct mm*)malloc(MMARRAYSIZE*sizeof(struct mm));
    if (!p_a_mm) {
        TESTALOGI("cannot alloc buffer p_a_mm, size %d \n", MMARRAYSIZE);
        return -1;
    }

	if (0 > load_memmap(pid, p_a_mm, &nmm)) {
		TESTALOGI("cannot read memory map\n");
		free(p_a_mm);
		return -1;
	}

	if (0 > getLibAddr(soname, lib, sizeof(lib), &libaddr, p_a_mm, nmm)) {
		TESTALOGI("cannot find lib: %s\n", soname);
		free(p_a_mm);
		return -1;
	}

	s = load_symtab(lib);
	if (!s) {
		TESTALOGI("cannot read symbol table\n");
		free(p_a_mm);
		return -1;
	}

    int succesCount = 0;
    for (int i = 0; i < count; i++) {
        result[i] = lookup_func_sym(s, symbol[i], &(addrArray[i]), &(sizeArray[i]));
        if (result[i] == 0) {
            if (sizeArray[i] >= 16) {
                addrArray[i] = addrArray[i] + libaddr;
                succesCount++;
            } else {
                result[i] = -1; // bypass
            }
            TESTALOGI("%s libAddr 0x%lx: *symbolAddr 0x%lx size 0x%lx", lib, libaddr, \
                                    addrArray[i], sizeArray[i]);
        }
    }

    free(p_a_mm);
    freeMMStructure(s);

    if (succesCount) {
	    return 0;
	} else {
	    return -1;
	}
}

int getFuctionOffsetBase(const char* libName) {
    int fd;
    fd = open(libName, O_RDONLY);
    if (-1 == fd) {
        TESTALOGI(" open %s error", libName);
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
        TESTALOGI("malloc fail");
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
            TESTALOGI("%s %s \n", libName, &(string_table[name_idx]));
            TESTALOGI("%s first_entry_addr = 0x%x  first_entry_offset = 0x%x \n", libName, first_entry_addr, first_entry_offset);
            break;
        }
    }
    return first_entry_addr - first_entry_offset;
}


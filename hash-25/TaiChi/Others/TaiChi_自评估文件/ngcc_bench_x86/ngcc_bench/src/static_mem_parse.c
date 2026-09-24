#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <elf.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libelf.h>
#include "bench_local.h"

static int parse_sections(Elf *elf, struct mem_profile *mem_prof)
{
    if(elf == NULL) {
        fprintf(stderr, "NULL elf.\n");
        return 1;
    }
    size_t shstrndx = 0;
    Elf64_Ehdr *ehdr = elf64_getehdr(elf);
    if(!ehdr){
        return 1;
    }
    shstrndx = ehdr->e_shstrndx;
    Elf_Scn *scn = NULL;

    while ((scn = elf_nextscn(elf, scn)) != NULL)
    {
        Elf64_Shdr *shdr = elf64_getshdr(scn);
        if (shdr == NULL)
            continue;

        const char *name = elf_strptr(elf, shstrndx, shdr->sh_name);
        if (!name) {
            continue;
        }
        if (strcmp(name, ".text") == 0) {
            mem_prof->text_size += shdr->sh_size;
        } 
        if ((strcmp(name, ".data") == 0) || (strcmp(name, ".rodata") == 0)) {
            mem_prof->data_size += shdr->sh_size;
        }

        if (strcmp(name, ".bss") == 0) {
            mem_prof->bss_size += shdr->sh_size;
        }
    }
    return 0;
}

static int inspect_member(Elf *elf, struct mem_profile *mem_prof)
{
    if(parse_sections(elf, mem_prof))
        return 1;

    return 0;
}

static int process_archive(int fd, Elf *arf, struct mem_profile *mem_prof)
{
    Elf_Cmd cmd = ELF_C_READ;
    Elf *member = NULL;

    while ((member = elf_begin(fd, cmd, arf)) != NULL)
    {
        if(!member)
            continue;
        if(inspect_member(member, mem_prof)) {
            // there are two empty objects in the archive file structure
            // do nothing
        };

        cmd = elf_next(member);
        elf_end(member);
    }
    return 0;
}

static int inspect_file(const char *path, struct mem_profile *mem_prof)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open failed.\n");
        return 1;
    }
    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF init failed.\n");
        return 1;
    }

    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) {
        fprintf(stderr, "elf_begin failed.\n");
        return 1;
    }

    if (elf_kind(elf) == ELF_K_AR) {
        if(process_archive(fd, elf, mem_prof)) {
            goto end;
        }
    }
    else if (elf_kind(elf) == ELF_K_ELF) {
        if(inspect_member(elf, mem_prof)) {
            goto end;
        }
    }
    else {
        printf("unknown file type.\n");
        return 1;
    }
end:
    elf_end(elf);
    close(fd);
    return 0;
}

int static_mem_parse(const char* lib_name, struct mem_profile *mem_prof) {
    return inspect_file(lib_name, mem_prof);
}
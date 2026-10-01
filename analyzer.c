#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>

Elf64_Off vaddr_to_offset(Elf64_Phdr *phdrs, int phnum, Elf64_Addr addr)
{
    int i;

    for (i = 0; i < phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD &&
            addr >= phdrs[i].p_vaddr &&
            addr < phdrs[i].p_vaddr + phdrs[i].p_filesz)
        {
            return phdrs[i].p_offset + (addr - phdrs[i].p_vaddr);
        }
    }

    return 0;
}

int main(void)
{
    int fd;
    Elf64_Ehdr ehdr;
    Elf64_Phdr *phdrs;
    Elf64_Shdr *shdrs;
    char *shstrtab;
    char *strtab;
    Elf64_Sym *symtab;
    unsigned char *main_bytes;
    unsigned char *start_bytes;

    Elf64_Addr text_addr = 0;
    Elf64_Off text_offset = 0;
    Elf64_Xword text_size = 0;

    Elf64_Addr main_addr = 0;
    Elf64_Xword main_size = 0;
    Elf64_Off main_offset;

    Elf64_Addr start_addr = 0;
    Elf64_Xword start_size = 0;
    Elf64_Off start_offset;

    Elf64_Addr calculated_main = 0;

    int i;
    int j;
    int sym_count;
    int found_main = 0;

    fd = open("hello", O_RDONLY);

    read(fd, &ehdr, sizeof(ehdr));

    printf("Entry point : 0x%lx\n", ehdr.e_entry);
    printf("PHDR offset : 0x%lx\n", ehdr.e_phoff);
    printf("PHDR count  : %d\n\n", ehdr.e_phnum);

    phdrs = malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));

    lseek(fd, ehdr.e_phoff, SEEK_SET);
    read(fd, phdrs, ehdr.e_phnum * sizeof(Elf64_Phdr));

    printf("===== LOAD SEGMENTS =====\n");

    for (i = 0; i < ehdr.e_phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD)
        {
            printf("LOAD %d : offset=0x%lx vaddr=0x%lx "
                   "filesz=0x%lx memsz=0x%lx\n",
                   i,
                   phdrs[i].p_offset,
                   phdrs[i].p_vaddr,
                   phdrs[i].p_filesz,
                   phdrs[i].p_memsz);
        }
    }

    shdrs = malloc(ehdr.e_shnum * sizeof(Elf64_Shdr));

    lseek(fd, ehdr.e_shoff, SEEK_SET);
    read(fd,
         shdrs,
         ehdr.e_shnum * sizeof(Elf64_Shdr));

    shstrtab = malloc(shdrs[ehdr.e_shstrndx].sh_size);

    lseek(fd,
          shdrs[ehdr.e_shstrndx].sh_offset,
          SEEK_SET);

    read(fd,
         shstrtab,
         shdrs[ehdr.e_shstrndx].sh_size);

    for (i = 0; i < ehdr.e_shnum; i++)
    {
        if (strcmp(&shstrtab[shdrs[i].sh_name], ".text") == 0)
        {
            text_addr = shdrs[i].sh_addr;
            text_offset = shdrs[i].sh_offset;
            text_size = shdrs[i].sh_size;
        }
    }

    printf("\n===== .TEXT =====\n");
    printf("Address : 0x%lx\n", text_addr);
    printf("Offset  : 0x%lx\n", text_offset);
    printf("Size    : 0x%lx\n", text_size);

    for (i = 0; i < ehdr.e_phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD &&
            text_addr >= phdrs[i].p_vaddr &&
            text_addr < phdrs[i].p_vaddr + phdrs[i].p_memsz)
        {
            printf("LOAD    : %d\n", i);
            printf("LOAD offset : 0x%lx\n", phdrs[i].p_offset);
            printf("LOAD vaddr  : 0x%lx\n", phdrs[i].p_vaddr);
        }
    }

    for (i = 0; i < ehdr.e_shnum; i++)
    {
        if (shdrs[i].sh_type == SHT_SYMTAB)
        {
            symtab = malloc(shdrs[i].sh_size);

            lseek(fd, shdrs[i].sh_offset, SEEK_SET);
            read(fd,
                 symtab,
                 shdrs[i].sh_size);

            sym_count = shdrs[i].sh_size / sizeof(Elf64_Sym);

            strtab = malloc(shdrs[shdrs[i].sh_link].sh_size);

            lseek(fd,
                  shdrs[shdrs[i].sh_link].sh_offset,
                  SEEK_SET);

            read(fd,
                 strtab,
                 shdrs[shdrs[i].sh_link].sh_size);

            for (j = 0; j < sym_count; j++)
            {
                if (strcmp(&strtab[symtab[j].st_name], "main") == 0)
                {
                    main_addr = symtab[j].st_value;
                    main_size = symtab[j].st_size;
                }

                if (strcmp(&strtab[symtab[j].st_name], "_start") == 0)
                {
                    start_addr = symtab[j].st_value;
                    start_size = symtab[j].st_size;
                }
            }

            free(strtab);
            free(symtab);

            break;
        }
    }

    printf("\n===== ENTRY =====\n");
    printf("Entry point : 0x%lx\n", ehdr.e_entry);
    printf("_start      : 0x%lx\n", start_addr);
    printf("_start size : 0x%lx\n", start_size);

    start_offset = vaddr_to_offset(
        phdrs,
        ehdr.e_phnum,
        start_addr
    );

    printf("Offset      : 0x%lx\n", start_offset);

    start_bytes = malloc(start_size);

    lseek(fd, start_offset, SEEK_SET);
    read(fd, start_bytes, start_size);

    printf("\n===== _START BYTES =====\n");

    for (i = 0; i < (int)start_size; i++)
    {
        printf("%02x ", start_bytes[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    printf("\n========================\n");

    /*
     * Search for:
     *
     * 48 8d 3d XX XX XX XX
     *
     * This is:
     *
     * lea displacement(%rip), %rdi
     *
     * The displacement points to main.
     */

    for (j = 0; j + 6 < (int)start_size; j++)
    {
        if (start_bytes[j] == 0x48 &&
            start_bytes[j + 1] == 0x8d &&
            start_bytes[j + 2] == 0x3d)
        {
            int32_t displacement;

            memcpy(&displacement,
                   &start_bytes[j + 3],
                   sizeof(displacement));

            calculated_main =
                start_addr + j + 7 + displacement;

            found_main = 1;

            printf("\n===== CALCULATED MAIN =====\n");
            printf("Instruction offset : 0x%x\n", j);
            printf("Instruction address: 0x%lx\n",
                   start_addr + j);
            printf("Next instruction   : 0x%lx\n",
                   start_addr + j + 7);
            printf("Displacement       : 0x%x\n",
                   displacement);
            printf("Calculated main    : 0x%lx\n",
                   calculated_main);

            break;
        }
    }

    if (!found_main)
    {
        printf("\n===== CALCULATED MAIN =====\n");
        printf("main not found\n");
    }

    free(start_bytes);

    printf("\n===== MAIN =====\n");
    printf("Address : 0x%lx\n", main_addr);

    main_offset = vaddr_to_offset(
        phdrs,
        ehdr.e_phnum,
        main_addr
    );

    printf("Offset  : 0x%lx\n", main_offset);
    printf("Size    : 0x%lx\n", main_size);

    printf("\n===== MAIN LOAD =====\n");

    for (i = 0; i < ehdr.e_phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD &&
            main_addr >= phdrs[i].p_vaddr &&
            main_addr < phdrs[i].p_vaddr + phdrs[i].p_filesz)
        {
            printf("LOAD     : %d\n", i);
            printf("Offset   : 0x%lx\n", phdrs[i].p_offset);
            printf("Vaddr    : 0x%lx\n", phdrs[i].p_vaddr);
            printf("FileSize : 0x%lx\n", phdrs[i].p_filesz);
            printf("MemSize  : 0x%lx\n", phdrs[i].p_memsz);

            printf("Flags    : ");

            if (phdrs[i].p_flags & PF_R)
                printf("R ");

            if (phdrs[i].p_flags & PF_W)
                printf("W ");

            if (phdrs[i].p_flags & PF_X)
                printf("E ");

            printf("\n");
        }
    }

    main_bytes = malloc(main_size);

    lseek(fd, main_offset, SEEK_SET);
    read(fd, main_bytes, main_size);

    printf("\n===== MAIN BYTES =====\n");

    for (i = 0; i < (int)main_size; i++)
    {
        printf("%02x ", main_bytes[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    printf("\n======================\n");

    free(main_bytes);
    free(shstrtab);
    free(shdrs);
    free(phdrs);

    close(fd);

    return 0;
}

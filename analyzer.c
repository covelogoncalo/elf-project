#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int fd;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;
    Elf64_Shdr shdr;
    Elf64_Shdr shstrtab;
    char *strtab;
    int i;

    Elf64_Addr text_addr = 0;
    Elf64_Off text_offset = 0;
    Elf64_Xword text_size = 0;

    unsigned char *text_data;
    Elf64_Xword j;

    fd = open("hello", O_RDONLY);

    if (fd == -1)
    {
        printf("Erreur ouverture\n");
        return 1;
    }

    read(fd, &ehdr, sizeof(ehdr));

    printf("Magic : %x %x %x %x\n",
        ehdr.e_ident[0],
        ehdr.e_ident[1],
        ehdr.e_ident[2],
        ehdr.e_ident[3]);

    printf("Program headers : %d\n", ehdr.e_phnum);
    printf("Program headers offset : %ld\n\n", ehdr.e_phoff);

    printf("Entry point : 0x%lx\n\n", ehdr.e_entry);

    /*
     * Program Headers
     */

    lseek(fd, ehdr.e_phoff, SEEK_SET);

    for (i = 0; i < ehdr.e_phnum; i++)
    {
        read(fd, &phdr, sizeof(phdr));

        printf("Program Header %d\n", i);

        if (phdr.p_type == PT_LOAD)
            printf("  Type   : LOAD\n");
        else if (phdr.p_type == PT_INTERP)
            printf("  Type   : INTERP\n");
        else if (phdr.p_type == PT_DYNAMIC)
            printf("  Type   : DYNAMIC\n");
        else if (phdr.p_type == PT_NOTE)
            printf("  Type   : NOTE\n");
        else
            printf("  Type   : OTHER\n");

        printf("  Flags  : ");

        if (phdr.p_flags & PF_R)
            printf("R");

        if (phdr.p_flags & PF_W)
            printf("W");

        if (phdr.p_flags & PF_X)
            printf("E");

        printf("\n");

        printf("  Offset : 0x%lx\n", phdr.p_offset);
        printf("  VAddr  : 0x%lx\n", phdr.p_vaddr);
        printf("  FileSz : 0x%lx\n", phdr.p_filesz);
        printf("  MemSz  : 0x%lx\n", phdr.p_memsz);

        if (phdr.p_type == PT_LOAD)
        {
            if (ehdr.e_entry >= phdr.p_vaddr &&
                ehdr.e_entry < phdr.p_vaddr + phdr.p_memsz)
            {
                printf("  --> ENTRY POINT\n");
            }
        }

        printf("\n");
    }

    /*
     * Section Headers
     */

    printf("Section headers : %d\n", ehdr.e_shnum);
    printf("Section headers offset : 0x%lx\n", ehdr.e_shoff);
    printf("Section header size : %d\n", ehdr.e_shentsize);
    printf("String table index : %d\n\n", ehdr.e_shstrndx);

    /*
     * Find .shstrtab
     */

    lseek(fd, ehdr.e_shoff +
               (ehdr.e_shstrndx * ehdr.e_shentsize),
          SEEK_SET);

    read(fd, &shstrtab, sizeof(shstrtab));

    strtab = malloc(shstrtab.sh_size);

    if (strtab == NULL)
    {
        printf("Erreur malloc\n");
        close(fd);
        return 1;
    }

    lseek(fd, shstrtab.sh_offset, SEEK_SET);
    read(fd, strtab, shstrtab.sh_size);

    /*
     * Read every section
     */

    lseek(fd, ehdr.e_shoff, SEEK_SET);

    for (i = 0; i < ehdr.e_shnum; i++)
    {
        read(fd, &shdr, sizeof(shdr));

        printf("Section Header %d\n", i);

        printf("  Name   : %s\n", strtab + shdr.sh_name);

        if (shdr.sh_type == SHT_PROGBITS)
            printf("  Type   : PROGBITS\n");
        else if (shdr.sh_type == SHT_NOBITS)
            printf("  Type   : NOBITS\n");
        else if (shdr.sh_type == SHT_SYMTAB)
            printf("  Type   : SYMTAB\n");
        else if (shdr.sh_type == SHT_STRTAB)
            printf("  Type   : STRTAB\n");
        else if (shdr.sh_type == SHT_DYNAMIC)
            printf("  Type   : DYNAMIC\n");
        else if (shdr.sh_type == SHT_RELA)
            printf("  Type   : RELA\n");
        else
            printf("  Type   : OTHER\n");

        printf("  Flags  : ");

        if (shdr.sh_flags & SHF_ALLOC)
            printf("A");

        if (shdr.sh_flags & SHF_WRITE)
            printf("W");

        if (shdr.sh_flags & SHF_EXECINSTR)
            printf("X");

        printf("\n");

        printf("  Offset : 0x%lx\n", shdr.sh_offset);
        printf("  Address: 0x%lx\n", shdr.sh_addr);
        printf("  Size   : 0x%lx\n", shdr.sh_size);

        /*
         * Find .text
         */

        if (strcmp(strtab + shdr.sh_name, ".text") == 0)
        {
            text_addr = shdr.sh_addr;
            text_offset = shdr.sh_offset;
            text_size = shdr.sh_size;

            printf("\n");
            printf("===== TEXT SECTION =====\n");
            printf("Offset : 0x%lx\n", text_offset);
            printf("Address: 0x%lx\n", text_addr);
            printf("Size   : 0x%lx\n", text_size);
            printf("========================\n");
        }

        printf("\n");
    }

    /*
     * Find the PT_LOAD containing .text
     */

    printf("===== TEXT LOAD SEGMENT =====\n");

    lseek(fd, ehdr.e_phoff, SEEK_SET);

    for (i = 0; i < ehdr.e_phnum; i++)
    {
        read(fd, &phdr, sizeof(phdr));

        if (phdr.p_type != PT_LOAD)
            continue;

        if (text_addr >= phdr.p_vaddr &&
            text_addr + text_size <= phdr.p_vaddr + phdr.p_memsz)
        {
            printf("Program Header : %d\n", i);
            printf("Offset         : 0x%lx\n", phdr.p_offset);
            printf("VAddr          : 0x%lx\n", phdr.p_vaddr);
            printf("FileSz         : 0x%lx\n", phdr.p_filesz);

            printf("Flags          : ");

            if (phdr.p_flags & PF_R)
                printf("R");

            if (phdr.p_flags & PF_W)
                printf("W");

            if (phdr.p_flags & PF_X)
                printf("E");

            printf("\n");
            printf("==============================\n");

            break;
        }
    }

    /*
     * Read .text bytes
     */

    text_data = malloc(text_size);

    if (text_data == NULL)
    {
        printf("Erreur malloc\n");
        free(strtab);
        close(fd);
        return 1;
    }

    lseek(fd, text_offset, SEEK_SET);

    read(fd, text_data, text_size);

    printf("\n===== TEXT BYTES =====\n");

    for (j = 0; j < text_size; j++)
    {
        printf("%02x ", text_data[j]);

        if ((j + 1) % 16 == 0)
            printf("\n");
    }

    printf("\n======================\n");

    free(text_data);
    free(strtab);
    close(fd);

    return 0;
}

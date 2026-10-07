#include <elf.h>
#include <stdio.h>
#include <stdlib.h>

long virtual_to_offset(Elf64_Phdr *phdrs, int count, Elf64_Addr vaddr)
{
	int i;

	for (i = 0; i < count; i++)
	{
		if (phdrs[i].p_type != PT_LOAD)
			continue;

		if (vaddr >= phdrs[i].p_vaddr &&
			vaddr < phdrs[i].p_vaddr + phdrs[i].p_filesz)
		{
			return phdrs[i].p_offset +
				(vaddr - phdrs[i].p_vaddr);
		}
	}

	return -1;
}

int main(int argc, char **argv)
{
	FILE *file;
	Elf64_Ehdr header;
	Elf64_Phdr *phdrs;
	int i;

	if (argc != 2)
	{
		printf("Usage: %s <elf>\n", argv[0]);
		return 1;
	}

	file = fopen(argv[1], "rb");
	if (file == NULL)
	{
		perror("fopen");
		return 1;
	}

	if (fread(&header, sizeof(header), 1, file) != 1)
	{
		printf("Erreur de lecture\n");
		fclose(file);
		return 1;
	}

	if (header.e_ident[EI_MAG0] != ELFMAG0 ||
		header.e_ident[EI_MAG1] != ELFMAG1 ||
		header.e_ident[EI_MAG2] != ELFMAG2 ||
		header.e_ident[EI_MAG3] != ELFMAG3)
	{
		printf("Ce fichier n'est pas un ELF\n");
		fclose(file);
		return 1;
	}

	printf("ELF valide\n");
	printf("Entry point : 0x%lx\n", header.e_entry);
	printf("Program headers : %u\n", header.e_phnum);
	printf("Section headers : %u\n", header.e_shnum);

	phdrs = malloc(sizeof(Elf64_Phdr) * header.e_phnum);
	if (phdrs == NULL)
	{
		printf("Erreur malloc\n");
		fclose(file);
		return 1;
	}

	if (fseek(file, header.e_phoff, SEEK_SET) != 0)
	{
		printf("Erreur de positionnement\n");
		free(phdrs);
		fclose(file);
		return 1;
	}

	if (fread(phdrs, sizeof(Elf64_Phdr), header.e_phnum, file)
		!= header.e_phnum)
	{
		printf("Erreur de lecture des program headers\n");
		free(phdrs);
		fclose(file);
		return 1;
	}

	printf("\nPT_LOAD:\n");

	for (i = 0; i < header.e_phnum; i++)
	{
		if (phdrs[i].p_type != PT_LOAD)
			continue;

		printf("\nPT_LOAD %d\n", i);
		printf("  Offset : 0x%lx\n", phdrs[i].p_offset);
		printf("  VAddr  : 0x%lx\n", phdrs[i].p_vaddr);
		printf("  FileSz : 0x%lx\n", phdrs[i].p_filesz);
		printf("  MemSz  : 0x%lx\n", phdrs[i].p_memsz);

		printf("  Flags  : ");

		if (phdrs[i].p_flags & PF_R)
			printf("R");
		if (phdrs[i].p_flags & PF_W)
			printf("W");
		if (phdrs[i].p_flags & PF_X)
			printf("X");

		printf("\n");
	}

	printf("\nEntry point:\n");
	printf("  VA     : 0x%lx\n", header.e_entry);

	long offset = virtual_to_offset(
		phdrs,
		header.e_phnum,
		header.e_entry
	);

	if (offset >= 0)
		printf("  Offset : 0x%lx\n", offset);
	else
		printf("  Offset : introuvable\n");

	free(phdrs);
	fclose(file);

	return 0;
}

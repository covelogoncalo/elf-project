#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_headers(FILE *file, Elf64_Ehdr *header, Elf64_Phdr **phdrs)
{
	if (fread(header, sizeof(*header), 1, file) != 1)
	{
		printf("Erreur de lecture ELF\n");
		return 0;
	}

	if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0)
	{
		printf("Ce fichier n'est pas un ELF\n");
		return 0;
	}

	*phdrs = malloc(sizeof(Elf64_Phdr) * header->e_phnum);
	if (*phdrs == NULL)
	{
		printf("Erreur malloc\n");
		return 0;
	}

	if (fseek(file, header->e_phoff, SEEK_SET) != 0)
	{
		printf("Erreur de positionnement\n");
		free(*phdrs);
		return 0;
	}

	if (fread(*phdrs, sizeof(Elf64_Phdr),
		header->e_phnum, file) != header->e_phnum)
	{
		printf("Erreur de lecture des program headers\n");
		free(*phdrs);
		return 0;
	}

	return 1;
}

int find_executable_segment(Elf64_Phdr *phdrs, int count)
{
	int i;

	for (i = 0; i < count; i++)
	{
		if (phdrs[i].p_type == PT_LOAD &&
			(phdrs[i].p_flags & PF_X))
		{
			return i;
		}
	}

	return -1;
}

int main(int argc, char **argv)
{
	FILE *file;
	Elf64_Ehdr header;
	Elf64_Phdr *phdrs;
	int index;

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

	if (!read_headers(file, &header, &phdrs))
	{
		fclose(file);
		return 1;
	}

	printf("ELF valide\n");
	printf("Entry point : 0x%lx\n", header.e_entry);

	index = find_executable_segment(phdrs, header.e_phnum);

	if (index < 0)
	{
		printf("Aucun segment executable trouve\n");
		free(phdrs);
		fclose(file);
		return 1;
	}

	printf("\nSegment executable : %d\n", index);
	printf("  Offset debut : 0x%lx\n", phdrs[index].p_offset);
	printf("  Offset fin   : 0x%lx\n",
		phdrs[index].p_offset + phdrs[index].p_filesz);
	printf("  VA debut     : 0x%lx\n", phdrs[index].p_vaddr);
	printf("  VA fin       : 0x%lx\n",
		phdrs[index].p_vaddr + phdrs[index].p_filesz);
	printf("  Taille       : 0x%lx\n", phdrs[index].p_filesz);

	if (header.e_entry >= phdrs[index].p_vaddr &&
		header.e_entry < phdrs[index].p_vaddr + phdrs[index].p_filesz)
	{
		printf("  Entry point : dans le segment executable\n");
	}
	else
	{
		printf("  Entry point : hors du segment executable\n");
	}

	free(phdrs);
	fclose(file);

	return 0;
}

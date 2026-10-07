#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	FILE *file;
	Elf64_Ehdr header;

	if (argc != 2)
	{
		printf("Usage: %s <elf>\n", argv[0]);
		return 1;
	}

	file = fopen(argv[1], "r+b");
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

	if (memcmp(header.e_ident, ELFMAG, SELFMAG) != 0)
	{
		printf("Ce fichier n'est pas un ELF\n");
		fclose(file);
		return 1;
	}

	printf("ELF valide\n");
	printf("Entry point actuel : 0x%lx\n", header.e_entry);

	fclose(file);

	return 0;
}

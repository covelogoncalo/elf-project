#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	FILE *file;
	Elf64_Ehdr header;
	Elf64_Addr new_entry;

	if (argc != 3)
	{
		printf("Usage: %s <elf> <new_entry>\n", argv[0]);
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

	new_entry = strtoul(argv[2], NULL, 16);

	printf("ELF valide\n");
	printf("Entry point actuel : 0x%lx\n", header.e_entry);
	printf("Nouvel entry point : 0x%lx\n", new_entry);

	header.e_entry = new_entry;

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		printf("Erreur de positionnement\n");
		fclose(file);
		return 1;
	}

	if (fwrite(&header, sizeof(header), 1, file) != 1)
	{
		printf("Erreur d'ecriture\n");
		fclose(file);
		return 1;
	}

	fclose(file);

	printf("Entry point modifie.\n");

	return 0;
}

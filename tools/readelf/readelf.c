#include "elf.h"
#include <stdio.h>

/* Overview:
 *   Check whether specified buffer is valid ELF data.
 *
 * Pre-Condition:
 *   The memory within [binary, binary+size) must be valid to read.
 *
 * Post-Condition:
 *   Returns 0 if 'binary' isn't an ELF, otherwise returns 1.
 */
int is_elf_format(const void *binary, size_t size) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	return size >= sizeof(Elf32_Ehdr) && ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
	       ehdr->e_ident[EI_MAG1] == ELFMAG1 && ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
	       ehdr->e_ident[EI_MAG3] == ELFMAG3;
}

/* Overview:
 *   Parse the sections from an ELF binary.
 *
 * Pre-Condition:
 *   The memory within [binary, binary+size) must be valid to read.
 *
 * Post-Condition:
 *   Return 0 if success. Otherwise return < 0.
 *   If success, output the address of every section in ELF.
 */

int readelf(const void *binary, size_t size) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;

	// Check whether `binary` is a ELF file.
	if (!is_elf_format(binary, size)) {
		fputs("not an elf file\n", stderr);
		return -1;
	}

	// Get the address of the section table, the number of section headers and the size of a
	// section header.
	const void *sh_table;
	Elf32_Half sh_entry_count;
	Elf32_Half sh_entry_size;
	/* Exercise 1.1: Your code here. (1/2) */
	sh_entry_size = ehdr->e_shentsize;//节头表表项大小
	sh_entry_count = ehdr->e_shnum;//节头表表项数
	sh_table = ehdr->e_shoff + binary;//得到节头表的位置
	// For each section header, output its index and the section address.
	// The index should start from 0.
	for (int i = 0; i < sh_entry_count; i++) { //开始对节头表遍历，得到每一个节头表，以描述每一个节头表对应的每一个节
		const Elf32_Shdr *shdr;
		unsigned int addr;
		/* Exercise 1.1: Your code here. (2/2) */
		shdr = (const Elf32_Shdr *)(sh_table + i*sh_entry_size);//得到每一个节头的地址,节头相当于对每一节的索引，里面装了该节的很多信息
		addr = shdr->sh_addr;//得到每一个节的地址
		printf("%d:0x%x\n", i, addr);//计算addr
	}

	return 0;
}

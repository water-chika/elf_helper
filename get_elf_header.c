#include <elf.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

int is_elf64_file(FILE* file);

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage:\n\tget_elf_header elf_file\n");
        exit(-1);
    }
    FILE* elf_file = fopen(argv[1], "r");
    assert(elf_file);
    
    Elf64_Ehdr header;
    fseek(elf_file, 0, SEEK_SET);
    fread(&header, sizeof(header), 1, elf_file);
    assert(is_elf64_file(elf_file));
    printf("file type : %d\n", header.e_type);
    printf("machine : %d\n", header.e_machine);
    printf("version : %d\n", header.e_version);
    printf("entry : 0x%lx\n", header.e_entry);
    printf("flags : 0x%x\n", header.e_flags);
    printf("program header number : %d\n", header.e_phnum);
    printf("section header number : %d\n", header.e_shnum);
    printf("name string section index : %d\n", header.e_shstrndx);

    Elf64_Shdr name_string_section_header;
    fseek(elf_file, header.e_shoff + header.e_shentsize * header.e_shstrndx, SEEK_SET);
    fread(&name_string_section_header, header.e_shentsize, 1, elf_file);

    char *name_string_table = (char*)malloc(name_string_section_header.sh_size);
    fseek(elf_file, name_string_section_header.sh_offset, SEEK_SET);
    fread(name_string_table, name_string_section_header.sh_size, 1, elf_file);
    for (int i = 0; i < header.e_shnum; i++)
    {
        Elf64_Shdr section_header;
        fseek(elf_file, header.e_shoff + header.e_shentsize*i, SEEK_SET);
        int count = fread(&section_header, header.e_shentsize, 1, elf_file);
        assert(count == 1);
        printf("\nsection %d:\n", i);
        printf("name offset : %d\n", section_header.sh_name);
        printf("name : %s\n", name_string_table + section_header.sh_name);
        printf("type : %d\n", section_header.sh_type);
        printf("flags : 0x%lx\n", section_header.sh_flags);
        printf("addr : 0x%lx\n", section_header.sh_addr);
        printf("offset : %ld\n", section_header.sh_offset);
        printf("size : %ld\n", section_header.sh_size);
        printf("link : %d\n", section_header.sh_link);
        printf("info : %d\n", section_header.sh_info);
        printf("addralign : %ld\n", section_header.sh_addralign);
        printf("entsize : %ld\n", section_header.sh_entsize);
    }

    for (int i = 0; i < header.e_phnum; i++)
    {
        Elf64_Phdr program_header;
        fseek(elf_file, header.e_phoff + header.e_phentsize*i, SEEK_SET);
        int count = fread(&program_header, header.e_phentsize, 1, elf_file);
        assert(count == 1);
        printf("\nprogram %d:\n", i);
        printf("type : %d\n", program_header.p_type);
        printf("flags : 0x%x\n", program_header.p_flags);
        printf("offset : %ld\n", program_header.p_offset);
        printf("vaddr : 0x%lx\n", program_header.p_vaddr);
        printf("paddr : 0x%lx\n", program_header.p_paddr);
        printf("filesz : %ld\n", program_header.p_filesz);
        printf("memsz : %ld\n", program_header.p_memsz);
        printf("align : %ld\n", program_header.p_align);
    }
    free(name_string_table);
    fclose(elf_file);
    return 0;
}


int is_elf64_file(FILE* file)
{
    fseek(file, 0, SEEK_SET);
    Elf64_Ehdr header;
    fread(&header, sizeof(header), 1, file);
    return (header.e_ident[EI_MAG0] == 0x7f && header.e_ident[EI_MAG1] == 'E' && header.e_ident[EI_MAG2] == 'L' && header.e_ident[EI_MAG3]=='F')
        && header.e_ident[EI_CLASS] == ELFCLASS64;
}

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

typedef uint8_t byte_t;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage:\n\tbin2elf binary_file elf_file\n");
        exit(-1);
    }

    char* bin_file_name = argv[1];
    char* elf_file_name = argv[2];
    FILE* bin_file = fopen(bin_file_name, "r");
    FILE* elf_file = fopen(elf_file_name, "w");

    Elf64_Ehdr elf_header = {0};
    Elf64_Shdr null_section_header = {};
    Elf64_Shdr name_string_section_header = {};
    Elf64_Shdr text_section_header = {};
    Elf64_Phdr program_header = {};

    fseek(bin_file, 0, SEEK_END);
    size_t text_size = ftell(bin_file);
    byte_t* text = (byte_t*)malloc(text_size);
    fseek(bin_file, 0, SEEK_SET);
    int count = fread(text, text_size, 1, bin_file);
    assert(count == 1);
    size_t name_string_table_size = 128;
    char* name_string_table = (char*)malloc(name_string_table_size);
    strcpy(name_string_table, "\0.shstrtab\0.text\0");
    strcpy(name_string_table + 1, ".shstrtab");
    strcpy(name_string_table + 11, ".text");

    int section_start_offset = 0x1000;
    
    elf_header.e_ident[EI_MAG0] = 0x7f;
    elf_header.e_ident[EI_MAG1] = 'E';
    elf_header.e_ident[EI_MAG2] = 'L';
    elf_header.e_ident[EI_MAG3] = 'F';
    elf_header.e_ident[EI_CLASS] = ELFCLASS64;
    elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
    elf_header.e_ident[EI_VERSION] = EV_CURRENT;
    elf_header.e_ident[EI_OSABI] = ELFOSABI_LINUX;
    elf_header.e_ident[EI_ABIVERSION] = 0;
    elf_header.e_type = ET_EXEC;
    elf_header.e_machine = EM_X86_64;
    elf_header.e_version = EV_CURRENT;
    elf_header.e_entry = 0x401000;
    elf_header.e_phoff = sizeof(Elf64_Ehdr);
    elf_header.e_shoff = section_start_offset + text_size + name_string_table_size;
    elf_header.e_flags = 0;
    elf_header.e_ehsize = sizeof(Elf64_Ehdr);
    elf_header.e_phentsize = sizeof(Elf64_Phdr);
    elf_header.e_phnum = 1;
    elf_header.e_shentsize = sizeof(Elf64_Shdr);
    elf_header.e_shnum = 3;
    elf_header.e_shstrndx = 1;

    name_string_section_header.sh_name = 1;
    name_string_section_header.sh_type = SHT_STRTAB;
    name_string_section_header.sh_flags = 0;
    name_string_section_header.sh_addr = 0;
    name_string_section_header.sh_offset = section_start_offset + text_size;
    name_string_section_header.sh_size = name_string_table_size;
    name_string_section_header.sh_addralign = 1;

    text_section_header.sh_name = 11;
    text_section_header.sh_type = SHT_PROGBITS;
    text_section_header.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    text_section_header.sh_addr = 0x401000;
    text_section_header.sh_offset = section_start_offset;
    text_section_header.sh_size = text_size;
    text_section_header.sh_link = SHN_UNDEF;
    text_section_header.sh_info = 0;
    text_section_header.sh_addralign = 1;
    text_section_header.sh_entsize = 0;

    program_header.p_type = PT_LOAD;
    program_header.p_flags = PF_X | PF_R;
    program_header.p_offset = section_start_offset;
    program_header.p_vaddr = 0x401000;
    program_header.p_paddr = 0x401000;
    program_header.p_filesz = text_size;
    program_header.p_memsz = text_size;
    program_header.p_align = 0x1000;

    fseek(elf_file, 0, SEEK_SET);
    count = fwrite(&elf_header, sizeof(elf_header), 1, elf_file);assert(count == 1);
    count = fwrite(&program_header, sizeof(program_header), 1, elf_file);assert(count == 1);

    fseek(elf_file, section_start_offset, SEEK_SET);
    count = fwrite(text, text_size, 1, elf_file); assert(count == 1);
    count = fwrite(name_string_table, name_string_table_size, 1, elf_file); assert(count == 1);
    count = fwrite(&null_section_header, sizeof(null_section_header), 1, elf_file); assert(count == 1);
    count = fwrite(&name_string_section_header, sizeof(name_string_section_header), 1, elf_file); assert(count == 1);
    count = fwrite(&text_section_header, sizeof(text_section_header), 1, elf_file); assert(count == 1);


    free(text);
    free(name_string_table);
    fclose(bin_file);
    fclose(elf_file);
}
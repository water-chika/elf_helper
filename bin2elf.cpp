#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <array>

#include "elf.hpp"

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
    fseek(bin_file, 0, SEEK_END);
    auto text = std::vector<uint8_t>(ftell(bin_file));
    if (text.size() > 0) {
        fseek(bin_file, 0, SEEK_SET);
        int count = fread(text.data(), text.size(), 1, bin_file);
        assert(count == 1);
    }
    FILE* elf_file = fopen(elf_file_name, "w");

    auto test_section = elf64::build::section{};
    auto name_string_table = elf64::build::string_table{
            ".shstrtab",
            ".text",
            ".strtab",
            ".symtab",
            "test"
    };
    auto name_section = elf64::build::section{name_string_table};
    auto symbol_table = elf64::build::symbol_table{
        elf64::symbol{
            .st_name = 1,
            .st_info = STT_FUNC,
            .st_other= STV_DEFAULT,
            .st_shndx= 3,
            .st_value= 0x401000,
            .st_size = sizeof(uint64_t),
        }
    };
    symbol_table.set_name_section_index(3);
    auto symbol_section = elf64::build::section{symbol_table};
    auto string_table = elf64::build::string_table{
        "test"
    };
    auto string_section = elf64::build::section{string_table};
    auto sections = elf64::build::sections{test_section, name_section, symbol_section, string_section};
    sections.set_name_index(0, 33);
    sections.set_name_index(1, 1);
    sections.set_name_index(2, 25);
    sections.set_name_index(3, 17);
    auto test_program = elf64::build::program{text};
    auto programs = elf64::build::programs{test_program};
    auto elf = elf64::build::elf{sections, programs};
    elf.set_name_section_index(1);
    elf.set_entry(0x40100);
    elf.set_type(ET_DYN);
    elf.write_to(elf_file);

    fclose(bin_file);
    fclose(elf_file);
}

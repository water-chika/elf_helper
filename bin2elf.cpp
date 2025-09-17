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
    fclose(bin_file);

    auto allocator = elf64::linear_allocator{};

    auto write_collect = std::vector<std::tuple<void*, size_t, size_t>>{};

    auto elf_header = elf64::elf_header{};
    auto elf_header_offset = allocator.allocate(sizeof(elf_header));
    assert(elf_header_offset == 0);
    write_collect.emplace_back(&elf_header,elf_header_offset, sizeof(elf_header));

    auto text_offset = allocator.allocate(text.size());
    write_collect.emplace_back(text.data(), text_offset, text.size()*sizeof(text[0]));

    auto section_headers = std::vector<elf64::section_header>();
    section_headers.emplace_back(); // null section
    section_headers.emplace_back();
    auto symbol_section_index = section_headers.size()-1;
    section_headers.emplace_back();
    auto section_name_section_index = section_headers.size()-1;
    section_headers.emplace_back();
    auto symbol_name_section_index = section_headers.size()-1;
    section_headers.emplace_back();
    auto text_section_index = section_headers.size()-1;

    auto section_headers_offset = allocator.allocate(sizeof(section_headers[0])*section_headers.size());

    write_collect.emplace_back(section_headers.data(), section_headers_offset, sizeof(section_headers[0])*section_headers.size());
    auto& section_name_section = section_headers[section_name_section_index];
    section_name_section.sh_addralign = 1;
    auto& symbol_section = section_headers[symbol_section_index];
    symbol_section.sh_addralign = 8;
    auto& symbol_name_section = section_headers[symbol_name_section_index];
    symbol_name_section.sh_addralign = 1;
    auto& text_section = section_headers[text_section_index];
    text_section.sh_addralign = 1;

    section_name_section.sh_type = SHT_STRTAB;

    auto strings = std::vector<char>{'\0'};

    section_name_section.sh_name = strings.size();
    strings.append_range(".shstrtab");

    text_section.sh_name = strings.size();
    strings.append_range(".text");

    symbol_name_section.sh_name = strings.size();
    strings.append_range(".strtab");

    symbol_section.sh_name = strings.size();
    strings.append_range(".symtab");

    auto symbol_test_name_index = static_cast<uint32_t>(strings.size());
    strings.append_range("test");

    auto strings_offset = allocator.allocate(sizeof(strings[0])*strings.size());
    write_collect.emplace_back(strings.data(), strings_offset, sizeof(strings[0])*strings.size());

    section_name_section.sh_offset = strings_offset;
    section_name_section.sh_size = sizeof(strings[0])*strings.size();
    symbol_name_section.sh_offset = strings_offset;
    symbol_name_section.sh_size = sizeof(strings[0]) * strings.size();
    symbol_name_section.sh_type = SHT_STRTAB;

    auto program_headers = std::vector<elf64::program_header>(1);
    auto& text_program = program_headers.back();

    auto program_headers_offset = allocator.allocate(sizeof(program_headers[0])*program_headers.size());
    write_collect.emplace_back(program_headers.data(), program_headers_offset, sizeof(program_headers[0])*program_headers.size());

    text_program.p_filesz = text.size();
    text_program.p_memsz = text.size();
    text_program.p_offset = text_offset;

    text_section.sh_offset = text_offset;
    text_section.sh_size = text.size();
    text_section.sh_type = SHT_PROGBITS;
    text_section.sh_flags = SHF_ALLOC | SHF_EXECINSTR;

    auto symbol_table = std::vector<elf64::symbol>{
        elf64::symbol{
            .st_name = 0,
            .st_info = 0,
            .st_other= STV_DEFAULT,
            .st_shndx= 0,
            .st_value= 0,
            .st_size = 0,
        },
        elf64::symbol{
            .st_name = symbol_test_name_index,
            .st_info = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC),
            .st_other= STV_DEFAULT,
            .st_shndx= static_cast<uint16_t>(text_section_index),
            .st_value= 0,
            .st_size = 7,
        }
    };
    auto last_local_symbol_index = 0;
    auto symbol_table_offset = allocator.allocate(sizeof(symbol_table[0])*symbol_table.size());
    write_collect.emplace_back(symbol_table.data(), symbol_table_offset, sizeof(symbol_table[0])*symbol_table.size());

    symbol_section.sh_offset = symbol_table_offset;
    symbol_section.sh_size = sizeof(symbol_table[0]) * symbol_table.size();
    symbol_section.sh_type = SHT_SYMTAB;
    symbol_section.sh_info = last_local_symbol_index+1;
    symbol_section.sh_link = symbol_name_section_index;
    symbol_section.sh_entsize = sizeof(symbol_table[0]);

    auto elf_header_helper = elf64::helper::elf_header{
        .type = ET_REL,
        .section_offset = section_headers_offset,
        .program_offset = program_headers_offset,
        .program_count = program_headers.size()
    };
    elf_header_helper.section_count = section_headers.size(),
    elf_header_helper.section_string_section_index = section_name_section_index;
    elf_header = elf_header_helper;

    FILE* elf_file = fopen(elf_file_name, "w");
    for (auto& write : write_collect) {
        auto& [data, offset, size] = write;
        auto res = fseek(elf_file, offset, SEEK_SET);
        assert(res == 0);
        res = fwrite(data, size, 1, elf_file);
        assert(size == 0 || res == 1);
    }
    fclose(elf_file);
}

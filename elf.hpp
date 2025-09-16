#pragma once

#include <vector>
#include <string>
#include <variant>
#include <numeric>
#include <algorithm>
#include <ranges>
#include <iostream>

#include "cpp_helper/cpp_helper.hpp"

#include <elf.h>

namespace elf64 {
    using addr = Elf64_Addr;
    using off = Elf64_Off;
    using section = Elf64_Section;
    using versym = Elf64_Versym;
    using byte = unsigned char;
    using half = uint16_t;
    using sword = int32_t;
    using word = uint32_t;
    using sxword = int64_t;
    using xword = uint64_t;

    using elf_header = Elf64_Ehdr;
    using program_header = Elf64_Phdr;
    using section_header = Elf64_Shdr;
    using symbol = Elf64_Sym;
    using relocation = Elf64_Rel;
    using dynamic_tag = Elf64_Dyn;
    using note_header = Elf64_Nhdr;

    class linear_allocator {
    public:
        size_t allocate(size_t size) {
            assert(m_next_address < std::numeric_limits<size_t>::max() - size);
            auto address = m_next_address;
            m_next_address += size;
            return address;
        }
    private:
        size_t m_next_address;
    };

    namespace build {

    class symbol_table {
    public:
        symbol_table() = default;
        symbol_table(std::initializer_list<symbol> symbols) : m_symbols{symbols} {}
        void set_offset(off offset){
            assert(offset != 0);
            m_offset = offset;
        }
        auto get_offset() {
            return m_offset;
        }
        size_t content_size() {
            return m_symbols.size() * sizeof(symbol);
        }
        void write_to(FILE* file) {
            fseek(file, m_offset, SEEK_SET);
            if (content_size() > 0) {
                auto count = fwrite(m_symbols.data(), content_size(), 1, file);assert(count == 1);
            }
        }
        void set_name_section_index(size_t i) {
            m_name_section_index = i;
        }
        auto name_section_index() const {
            return m_name_section_index;
        }
        auto entry_count() const {
            return m_symbols.size();
        }
    private:
        size_t m_offset;
        size_t m_name_section_index;
        std::vector<symbol> m_symbols;
    };

    class string_table {
    public:
        string_table() = default;
        template<typename T>
            requires std::convertible_to<T,std::string>
        string_table(std::initializer_list<T> list) : m_strings(list.size()) {
            std::ranges::copy(
                    list,
                    m_strings.begin()
                    );

        }
        string_table(std::vector<std::string> strings) : m_strings{strings} {}
        void set_offset(off offset){
            assert(offset != 0);
            m_offset = offset;
        }
        auto get_offset() {
            return m_offset;
        }
        size_t content_size() {
            return std::transform_reduce(
                    m_strings.begin(),
                    m_strings.end(),
                    1,
                    std::plus<void>{},
                    [](auto& str) {
                        return str.size()+1;
                    }
                    );
        }
        void write_to(FILE* file) {
            fseek(file, m_offset, SEEK_SET);
            auto count = fwrite("", 1, 1, file);assert(count == 1);
            std::ranges::for_each(
                    m_strings,
                    [file](auto& str) {
                        auto count = fwrite(str.data(), str.size()+1, 1, file);assert(count == 1);
                    }
                    );
        }
        auto entry_count() {
            return m_strings.size();
        }
    private:
        size_t m_offset;
        std::vector<std::string> m_strings;
    };

    class program_bits {
        program_bits() = default;
        program_bits(size_t offset) : m_program_offset{offset} {}
        void set_offset(off offset){
            m_offset = offset;
        }
        auto get_offset() {
            return m_offset;
        }
        size_t content_size() {
            return 0;
        }
        void write_to(FILE* file) {
            return;
        }
        auto entry_count() {
            return 0;
        }
    private:
        size_t m_offset;
        size_t m_program_offset;
    };

    class section {
    public:
        section() = default;
        section(std::variant<symbol_table, string_table>  content) : m_content{content} {}
        void set_offset(off offset){
            assert(offset != 0);
            m_offset = offset;
            std::visit(
                    [offset](auto& content){
                        content.set_offset(offset);
                    },
                    m_content
                    );
        }
        auto get_offset() {
            return m_offset;
        }
        auto content_size() {
            return std::visit(
                    [](auto content){
                        return content.content_size();
                    },
                    m_content
                    );
        }
        auto next_offset() {
            return get_offset() + content_size();
        }

        void write_to(FILE* file) {
            std::visit(
                    [file](auto& content){
                        content.write_to(file);
                    },
                    m_content
                    );
        }
        auto type() {
            return std::visit(
                    cpp_helper::overloads{
                        [](const symbol_table& content) { return SHT_SYMTAB; },
                        [](const string_table&) { return SHT_STRTAB; }
                    },
                    m_content
                    );
        }
        auto addr() {
            return 0;
        }
        auto entry_size() {
            return std::visit(
                    cpp_helper::overloads{
                        [](const symbol_table& content) -> size_t { return sizeof(symbol); },
                        [](const string_table& content) -> size_t { return 0; }
                    },
                    m_content
                    );
        }
        auto entry_count() {
            return std::visit(
                    cpp_helper::overloads{
                        [](const symbol_table& content) -> size_t { return content.entry_count(); },
                        [](const string_table& content) -> size_t { return 0; }
                    },
                    m_content
                    );
        }
        auto name_section_index() {
            return std::visit(
                    cpp_helper::overloads{
                        [](const symbol_table& content) -> size_t { return content.name_section_index(); },
                        [](const string_table& content) -> size_t { return 0; }
                    },
                    m_content
                    );
        }
    private:
        off m_offset;
        std::variant<symbol_table, string_table> m_content;
    };

    class program {
    public:
        program() = default;
        program(std::vector<uint8_t> binary_codes) : m_binary_codes{binary_codes} {}
        void set_offset(off offset){
            assert(offset != 0);
            m_offset = offset;
        }
        auto get_offset() {
            return m_offset;
        }
        auto content_size() {
            return m_binary_codes.size();
        }
        auto next_offset() {
            return get_offset() + content_size();
        }
        void write_to(FILE* file) {
            fseek(file, m_offset, SEEK_SET);
            if (content_size() > 0) {
                auto count = fwrite(m_binary_codes.data(), content_size(), 1, file);assert(count == 1);
            }
        }
    private:
        size_t m_offset;
        std::vector<uint8_t> m_binary_codes;
    };

    class sections {
    public:
        sections() = default;
        sections(section sect) : m_sections{sect},m_name_indices(1) {}
        sections(std::initializer_list<section> sects) : m_sections{sects},m_name_indices(sects.size()) {}
        sections(std::vector<section> sects) : m_sections{sects}, m_name_indices(sects.size()) {}
        void set_offset(off offset){
            assert(offset != 0);
            m_offset = offset;
            offset += sizeof(section_header)*m_sections.size();
            for (auto& sect : m_sections) {
                sect.set_offset(offset);
                offset += sect.content_size();
            }
        }
        auto get_offset() {
            return m_offset;
        }
        auto content_size() {
            return std::transform_reduce(
                    m_sections.begin(),
                    m_sections.end(),
                    sizeof(section_header)*m_sections.size(),
                    std::plus<void>{},
                    [](auto& sect) {
                        return sect.content_size();
                    }
                    );
        }
        auto next_offset() {
            return get_offset() + content_size();
        }
        void write_headers_to(FILE* file) {
            for (int i = 0; i < m_sections.size(); i++) {
                auto& sect = m_sections[i];
                section_header header{};
                header.sh_name = m_name_indices[i];
                header.sh_type = sect.type();
                header.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
                header.sh_addr = sect.addr();
                header.sh_offset = sect.get_offset();
                header.sh_size = sect.content_size();
                header.sh_link = sect.name_section_index();
                header.sh_info = sect.entry_count();
                header.sh_addralign = 1;
                header.sh_entsize = sect.entry_size();
                auto count = fwrite(&header, sizeof(header), 1, file);assert(count == 1);
            }
        }

        void write_contents_to(FILE* file) {
            for (auto& sect : m_sections) {
                sect.write_to(file);
            }
        }

        void write_to(FILE* file) {
            fseek(file, m_offset, SEEK_SET);
            write_headers_to(file);
            write_contents_to(file);
        }

        auto size() {
            return m_sections.size();
        }
        void set_name_index(size_t section_index, size_t i) {
            m_name_indices[section_index] = i;
        }
    private:
        size_t m_offset;
        std::vector<section> m_sections;
        std::vector<size_t> m_name_indices;
    };

    class programs {
    public:
        programs() = default;
        programs(program prog) : m_programs{prog} {}
        programs(std::vector<program> progs) : m_programs{progs} {}
        void set_offset(off offset){
            assert(offset != 0);
            m_offset = offset;
            offset += sizeof(program_header) * m_programs.size();
            for (int i = 0; i < m_programs.size(); i++) {
                auto& prog = m_programs[i];
                prog.set_offset(offset);
                offset = prog.next_offset();
            }
        }
        auto get_offset() {
            return m_offset;
        }
        auto content_size() {
            return std::transform_reduce(
                    m_programs.begin(),
                    m_programs.end(),
                    sizeof(program_header)*m_programs.size(),
                    std::plus<void>{},
                    [](auto& prog) {
                        return prog.content_size();
                    }
                    );
        }
        auto next_offset() {
            return get_offset() + content_size();
        }

        void write_headers_to(FILE* file) {
            for (int i = 0; i < m_programs.size(); i++) {
                auto& prog = m_programs[i];
                program_header header{};
                header.p_type = PT_LOAD;
                header.p_flags = PF_X | PF_R;
                header.p_offset = prog.get_offset();
                header.p_vaddr = 0x401000;
                header.p_paddr = 0x401000;
                header.p_filesz = prog.content_size();
                header.p_memsz = prog.content_size();
                header.p_align = 0x1000;
                auto count = fwrite(&header, sizeof(header), 1, file);assert(count == 1);
            }
        }

        void write_contents_to(FILE* file) {
            for (auto& prog : m_programs) {
                prog.write_to(file);
            }
        }

        void write_to(FILE* file) {
            fseek(file, m_offset, SEEK_SET);
            write_headers_to(file);
            write_contents_to(file);
        }
        auto size() {
            return m_programs.size();
        }
    private:
        size_t m_offset;
        std::vector<program> m_programs;
    };

    class elf {
    public:
        elf() = default;
        elf(
            sections sections,
            programs programs
        ) : m_sections{sections}, m_programs{programs}
        {
            set_offset(0);
        }
        void set_offset(size_t offset) {
            assert(offset == 0);
            m_sections.set_offset(sizeof(elf_header));
            m_programs.set_offset(m_sections.next_offset());

        }
        auto get_offset() {
            return 0;
        }
        auto content_size() {
            return sizeof(elf_header) + m_sections.content_size() + m_programs.content_size();
        }
        auto next_offset() {
            return get_offset() + content_size();
        }

        void write_header_to(FILE* file) {
            elf64::elf_header elf_header = {};
            elf_header.e_ident[EI_MAG0] = 0x7f;
            elf_header.e_ident[EI_MAG1] = 'E';
            elf_header.e_ident[EI_MAG2] = 'L';
            elf_header.e_ident[EI_MAG3] = 'F';
            elf_header.e_ident[EI_CLASS] = ELFCLASS64;
            elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
            elf_header.e_ident[EI_VERSION] = EV_CURRENT;
            elf_header.e_ident[EI_OSABI] = ELFOSABI_LINUX;
            elf_header.e_ident[EI_ABIVERSION] = 0;
            elf_header.e_type = m_type;
            elf_header.e_machine = EM_X86_64;
            elf_header.e_version = EV_CURRENT;
            elf_header.e_entry = m_entry;
            elf_header.e_phoff = m_programs.get_offset();
            elf_header.e_shoff = m_sections.get_offset();
            elf_header.e_flags = 0;
            elf_header.e_ehsize = sizeof(elf_header);
            elf_header.e_phentsize = sizeof(program_header);
            elf_header.e_phnum = m_programs.size();
            elf_header.e_shentsize = sizeof(section_header);
            elf_header.e_shnum = m_sections.size();
            elf_header.e_shstrndx = m_section_string_section_index;

            fseek(file, 0, SEEK_SET);
            auto count = fwrite(&elf_header, sizeof(elf_header), 1, file);assert(count == 1);
        }
        void write_to(FILE* file) {
            write_header_to(file);
            m_sections.write_to(file);
            m_programs.write_to(file);
        }
        void set_name_section_index(uint32_t i) {
            m_section_string_section_index = i;
        }
        void set_type(uint16_t type) {
            m_type = type;
        }
        void set_entry(addr entry_addr) {
            m_entry = entry_addr;
        }
    private:
        uint16_t m_type;
        addr m_entry;
        uint32_t m_section_string_section_index;
        sections m_sections;
        programs m_programs;
    };
    }
}

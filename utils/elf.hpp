#pragma once

#include <vector>
#include <string>
#include <variant>
#include <numeric>

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

    namespace build {

    class symbol_table {
    public:
        size_t content_size() {
            return m_symbols.size() * sizeof(symbol);
        }
    private:
        std::vector<symbol> m_symbols;
    };

    class string_table {
    public:
        size_t content_size() {
            return std::transform_reduce(
                    m_strings.begin(),
                    m_strings.end(),
                    0,
                    std::plus<void>{},
                    [](auto& str) {
                        return str.size();
                    }
                    );
        }
    private:
        std::vector<std::string> m_strings;
    };

    class section {
    public:
        void set_offset(off offset){
            m_offset = offset;
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
        }
    private:
        off m_offset;
        std::variant<symbol_table, string_table> m_content;
    };

    class program {
    public:
        void set_offset(off offset){
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
        }
    private:
        size_t m_offset;
        std::vector<uint8_t> m_binary_codes;
    };

    class sections {
    public:
        void set_offset(off offset){
            m_offset = offset;
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
                header.sh_name = 0;assert(0);
                header.sh_type = SHT_PROGBITS;
                header.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
                header.sh_addr = 0x401000;
                header.sh_offset = sect.get_offset();
                header.sh_size = sect.content_size();
                header.sh_link = SHN_UNDEF;
                header.sh_info = 0;
                header.sh_addralign = 1;
                header.sh_entsize = 0;
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
    private:
        size_t m_offset;
        std::vector<section> m_sections;
    };

    class programs {
    public:
        void set_offset(off offset){
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
        elf(
            sections sections,
            programs programs
        ) : m_sections{sections}, m_programs{programs}
        {}
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
            elf_header.e_type = ET_DYN;
            elf_header.e_machine = EM_X86_64;
            elf_header.e_version = EV_CURRENT;
            elf_header.e_entry = 0x401000;
            elf_header.e_phoff = m_programs.get_offset();
            elf_header.e_shoff = m_sections.get_offset();
            elf_header.e_flags = 0;
            elf_header.e_ehsize = sizeof(elf_header);
            elf_header.e_phentsize = sizeof(program_header);
            elf_header.e_phnum = m_programs.size();
            elf_header.e_shentsize = sizeof(section_header);
            elf_header.e_shnum = m_sections.size();
            elf_header.e_shstrndx = 0; assert(0);

            fseek(file, 0, SEEK_SET);
            auto count = fwrite(&elf_header, sizeof(elf_header), 1, file);assert(count == 1);
        }
        void write_to(FILE* file) {
            write_header_to(file);
            m_sections.write_to(file);
            m_programs.write_to(file);
        }
    private:
        sections m_sections;
        programs m_programs;
    };
    }
}

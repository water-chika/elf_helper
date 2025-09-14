#pragma once

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
}

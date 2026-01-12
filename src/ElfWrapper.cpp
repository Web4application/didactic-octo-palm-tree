#include "ElfWrapper.h"
#include <elfio/elfio.hpp>
#include <iostream>

ElfWrapper::ElfWrapper(const std::string& path) {
    ELFIO::elfio reader;
    if (!reader.load(path)) {
        std::cerr << "Failed to load ELF: " << path << "\n";
        m_loaded = false;
        return;
    }
    m_loaded = true;

    // Collect symbols (static + dynamic)
    const ELFIO::symbol_section_accessor* sym_acc = nullptr;
    for (auto sec : reader.sections) {
        if (sec->get_type() == ELFIO::SHT_SYMTAB || sec->get_type() == ELFIO::SHT_DYNSYM) {
            sym_acc = new ELFIO::symbol_section_accessor(reader, sec);
            for (unsigned int i = 0; i < sym_acc->get_symbols_num(); ++i) {
                std::string name;
                ELFIO::Elf64_Addr value = 0;
                ELFIO::Elf_Xword size = 0;
                unsigned char bind = 0;
                unsigned char type = 0;
                ELFIO::Elf_Half section = 0;
                unsigned char other = 0;

                sym_acc->get_symbol(i, name, value, size, bind, type, section, other);
                if (!name.empty()) {
                    m_symbols[name] = static_cast<uint64_t>(value);
                }
            }
            delete sym_acc;
            sym_acc = nullptr;
        }
    }

    // Find BSS section start
    for (auto sec : reader.sections) {
        if (sec->get_type() == ELFIO::SHT_NOBITS) { // typically .bss
            if (sec->get_name() == ".bss") {
                m_bss = static_cast<uint64_t>(sec->get_address());
                break;
            }
        }
    }

    // GOT resolution (demo): try to find .got/.got.plt section addresses
    for (auto sec : reader.sections) {
        auto name = sec->get_name();
        if (name == ".got" || name == ".got.plt") {
            // We don’t have per-symbol mapping here without relocations,
            // but we can store the section base as a placeholder.
            m_got[name] = static_cast<uint64_t>(sec->get_address());
        }
    }
}

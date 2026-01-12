#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

class ElfWrapper {
public:
    explicit ElfWrapper(const std::string& path);

    bool loaded() const { return m_loaded; }

    // Base address (for PIE or assumed load base). For demo, we’ll allow setting it.
    void setBase(uint64_t base) { m_base = base; }
    uint64_t getBase() const { return m_base; }

    // Symbol table: name -> value (address/offset)
    const std::unordered_map<std::string, uint64_t>& symbols() const { return m_symbols; }

    // GOT entries: name -> address (demo: we’ll try to resolve via dynamic symbols/relocations if present)
    const std::unordered_map<std::string, uint64_t>& got() const { return m_got; }

    // BSS start address (if present)
    uint64_t bss() const { return m_bss; }

    // Variable prefix used by CRAX-style formatting
    const std::string& getVarPrefix() const { return m_varPrefix; }
    void setVarPrefix(const std::string& p) { m_varPrefix = p; }

private:
    bool m_loaded = false;
    uint64_t m_base = 0;
    uint64_t m_bss = 0;
    std::string m_varPrefix = "elf";

    std::unordered_map<std::string, uint64_t> m_symbols;
    std::unordered_map<std::string, uint64_t> m_got;
};

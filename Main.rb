#include <iostream>
#include <vector>
#include <string>
#include "ElfWrapper.h"
#include "CRAXExpr.h"

using namespace kubu;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: elf_demo <path-to-elf>\n";
        return 1;
    }

    ElfWrapper elf(argv[1]);
    if (!elf.loaded()) {
        std::cerr << "ELF not loaded.\n";
        return 1;
    }

    // Configure base and prefix (demo)
    elf.setBase(0x400000);
    elf.setVarPrefix("elf");

    // Show a symbol if present
    std::string symName = "read"; // change to a symbol that exists in your ELF
    auto it = elf.symbols().find(symName);
    if (it != elf.symbols().end()) {
        auto base = klee::ConstantExpr::create(elf.getBase(), klee::Expr::Int64);
        auto off  = klee::ConstantExpr::create(it->second, klee::Expr::Int64);

        auto expr = BaseOffsetExpr::alloc(base, off, elf.getVarPrefix() + "_base",
                                          elf.getVarPrefix() + ".sym['" + symName + "']");
        auto boe  = klee::dyn_cast<BaseOffsetExpr>(expr.get());

        std::cout << "[BaseOffsetExpr] " << boe->toString() << "\n";
        std::cout << "Value: 0x" << std::hex << boe->getZExtValue() << "\n";
    } else {
        std::cout << "Symbol '" << symName << "' not found in ELF.\n";
    }

    // BSS example
    {
        auto base = klee::ConstantExpr::create(elf.getBase(), klee::Expr::Int64);
        auto off  = klee::ConstantExpr::create(elf.bss(), klee::Expr::Int64);
        auto expr = BaseOffsetExpr::alloc(base, off, elf.getVarPrefix() + "_base",
                                          elf.getVarPrefix() + ".bss()");
        auto boe  = klee::dyn_cast<BaseOffsetExpr>(expr.get());
        std::cout << "[BSS] " << boe->toString() << "\n";
        std::cout << "Value: 0x" << std::hex << boe->getZExtValue() << "\n";
    }

    // ByteVectorExpr payload
    {
        std::vector<uint8_t> payload = { 'H', 'e', 'l', 'l', 'o', '!', '\n' };
        auto expr = ByteVectorExpr::create(payload);
        auto bve  = klee::dyn_cast<ByteVectorExpr>(expr.get());
        std::cout << "[ByteVectorExpr] " << bve->toString() << "\n";
    }

    // PlaceholderExpr metadata
    {
        auto expr = PlaceholderExpr<std::string>::create("Exploit metadata: stage=1");
        auto pe   = klee::dyn_cast<PlaceholderExpr<std::string>>(expr.get());
        std::cout << "[PlaceholderExpr] " << pe->getUserData() << "\n";
    }

    // LambdaExpr action
    {
        auto expr = LambdaExpr::create([]() {
            std::cout << "[LambdaExpr] Running custom action...\n";
        });
        auto le = klee::dyn_cast<LambdaExpr>(expr.get());
        (*le)();
    }

    return 0;
}

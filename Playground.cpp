#include <iostream>
#include <vector>
#include <string>
#include <klee/Expr.h>
#include "CRAXExpr.h"   // the header you shared

using namespace kubu;

int main() {
    // 1. ConstantExpr + AddExpr
    auto a = klee::ConstantExpr::create(0x400000, klee::Expr::Int64);
    auto b = klee::ConstantExpr::create(0x1060, klee::Expr::Int64);
    auto sum = klee::AddExpr::create(a, b);
    auto result = dyn_cast<klee::ConstantExpr>(sum);
    std::cout << "[AddExpr] Base + Offset = 0x" 
              << std::hex << result->getZExtValue() << std::endl;

    // 2. BaseOffsetExpr (pretend offset from ELF symbol)
    auto boe = BaseOffsetExpr::alloc(a, b, "target_base", "elf.sym['read']");
    std::cout << "[BaseOffsetExpr] " << dyn_cast<BaseOffsetExpr>(boe)->toString() 
              << std::endl;

    // 3. ByteVectorExpr (raw payload)
    std::vector<uint8_t> payload = { 'H', 'e', 'l', 'l', 'o' };
    auto bve = ByteVectorExpr::create(payload);
    std::cout << "[ByteVectorExpr] " << dyn_cast<ByteVectorExpr>(bve)->toString() 
              << std::endl;

    // 4. PlaceholderExpr (store metadata)
    auto pe = PlaceholderExpr<std::string>::create("Exploit metadata here");
    std::cout << "[PlaceholderExpr] " 
              << dyn_cast<PlaceholderExpr<std::string>>(pe)->getUserData() 
              << std::endl;

    // 5. LambdaExpr (custom action)
    auto le = LambdaExpr::create([]() {
        std::cout << "[LambdaExpr] Running custom exploit action!" << std::endl;
    });
    (*dyn_cast<LambdaExpr>(le))();  // invoke callback

    return 0;
}

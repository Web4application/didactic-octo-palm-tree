#pragma once
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <iomanip>

// Minimal klee-like scaffolding
namespace klee {
    class Expr {
    public:
        enum Kind { Constant, InvalidKind };
        enum Width { InvalidWidth, Int64 };
        virtual ~Expr() = default;
        virtual Kind getKind() const = 0;
        virtual Width getWidth() const = 0;
        virtual unsigned getNumKids() const = 0;
        virtual Expr* getKid(unsigned) const = 0;
        virtual Expr* rebuild(Expr*[]) const = 0;
    };

    template <typename T>
    class ref {
        T* ptr;
    public:
        ref() : ptr(nullptr) {}
        explicit ref(T* p) : ptr(p) {}
        T* get() const { return ptr; }
        T* operator->() const { return ptr; }
        operator bool() const { return ptr != nullptr; }
    };

    template <typename T, typename U>
    T* dyn_cast(U* p) { return dynamic_cast<T*>(p); }

    class ConstantExpr : public Expr {
        uint64_t v;
        Width w;
    public:
        static ref<ConstantExpr> create(uint64_t val, Width width) {
            return ref<ConstantExpr>(new ConstantExpr(val, width));
        }
        ConstantExpr(uint64_t val, Width width) : v(val), w(width) {}
        Kind getKind() const override { return Constant; }
        Width getWidth() const override { return w; }
        unsigned getNumKids() const override { return 0; }
        Expr* getKid(unsigned) const override { return nullptr; }
        Expr* rebuild(Expr*[]) const override { return nullptr; }
        uint64_t getZExtValue() const { return v; }
        ref<ConstantExpr> Add(const ref<ConstantExpr>& other) const {
            return create(v + other->getZExtValue(), w);
        }
    };

    class AddExpr : public Expr {
    protected:
        ref<ConstantExpr> left, right;
    public:
        AddExpr(const ref<ConstantExpr>& l, const ref<ConstantExpr>& r) : left(l), right(r) {}
        static ref<Expr> create(const ref<ConstantExpr>& l, const ref<ConstantExpr>& r) {
            return ref<Expr>(new AddExpr(l, r));
        }
        Kind getKind() const override { return Constant; }
        Width getWidth() const override { return Int64; }
        unsigned getNumKids() const override { return 2; }
        Expr* getKid(unsigned i) const override {
            if (i == 0) return left.get();
            if (i == 1) return right.get();
            return nullptr;
        }
        Expr* rebuild(Expr*[]) const override { return nullptr; }
    };
}

// Formatting helpers
inline std::string format(const char* fmt, const char* a, const char* b) {
    std::ostringstream oss;
    oss << a << ".sym['" << b << "']";
    return oss.str();
}
inline std::string format(const char* fmt, const char* a) {
    std::ostringstream oss;
    oss << a << ".bss()";
    return oss.str();
}
inline std::string format_hex(uint64_t v) {
    std::ostringstream oss;
    oss << "0x" << std::hex << v;
    return oss.str();
}
template <typename It>
std::string toByteString(It first, It last) {
    std::ostringstream oss;
    for (auto it = first; it != last; ++it) {
        uint8_t c = *it;
        oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

namespace kubu {

// BaseOffsetExpr
class BaseOffsetExpr : public klee::AddExpr {
public:
    enum class BaseType { SYM, GOT, BSS, VAR };

    static klee::ref<klee::Expr> alloc(const klee::ref<klee::ConstantExpr>& lce,
                                       const klee::ref<klee::ConstantExpr>& rce,
                                       const std::string& strBase,
                                       const std::string& strOffset) {
        return klee::ref<klee::Expr>(new BaseOffsetExpr(lce, rce, strBase, strOffset));
    }

    std::string toString() const {
        std::string strLeft = m_strBase;
        std::string strRight;
        if (m_strOffset.size()) {
            strRight = m_strOffset;
        } else {
            auto rce = klee::dyn_cast<klee::ConstantExpr>(right.get());
            strRight = format_hex(rce->getZExtValue());
        }
        return strLeft + " + " + strRight;
    }

    uint64_t getZExtValue() const {
        auto lce = klee::dyn_cast<klee::ConstantExpr>(getKid(0));
        auto rce = klee::dyn_cast<klee::ConstantExpr>(getKid(1));
        return lce->getZExtValue() + rce->getZExtValue();
    }

private:
    BaseOffsetExpr(const klee::ref<klee::ConstantExpr>& lce,
                   const klee::ref<klee::ConstantExpr>& rce,
                   const std::string& strBase,
                   const std::string& strOffset)
        : klee::AddExpr(lce, rce), m_strBase(strBase), m_strOffset(strOffset) {}

    std::string m_strBase;
    std::string m_strOffset;
};

// ByteVectorExpr
class ByteVectorExpr : public klee::Expr {
public:
    static klee::ref<klee::Expr> create(const std::vector<uint8_t>& bytes) {
        return klee::ref<klee::Expr>(new ByteVectorExpr(bytes));
    }
    Kind getKind() const override { return Constant; }
    Width getWidth() const override { return static_cast<Width>(8 * m_bytes.size()); }
    unsigned getNumKids() const override { return 0; }
    klee::Expr* getKid(unsigned) const override { return nullptr; }
    klee::Expr* rebuild(klee::Expr*[]) const override { return nullptr; }
    std::string toString() const {
        return std::string("b'") + toByteString(m_bytes.begin(), m_bytes.end()) + "'";
    }
private:
    explicit ByteVectorExpr(const std::vector<uint8_t>& bytes) : m_bytes(bytes) {}
    std::vector<uint8_t> m_bytes;
};

// PlaceholderExpr
template <typename T>
class PlaceholderExpr : public klee::Expr {
public:
    static klee::ref<klee::Expr> create(const T& userData) {
        return klee::ref<klee::Expr>(new PlaceholderExpr(userData));
    }
    Kind getKind() const override { return InvalidKind; }
    Width getWidth() const override { return InvalidWidth; }
    unsigned getNumKids() const override { return 0; }
    klee::Expr* getKid(unsigned) const override { return nullptr; }
    klee::Expr* rebuild(klee::Expr*[]) const override { return nullptr; }
    const T& getUserData() const { return m_userData; }
private:
    explicit PlaceholderExpr(const T& userData) : m_userData(userData) {}
    T m_userData;
};

// LambdaExpr
class LambdaExpr : public klee::Expr {
    using CallbackType = std::function<void()>;
public:
    static klee::ref<klee::Expr> create(CallbackType cb) {
        return klee::ref<klee::Expr>(new LambdaExpr(std::move(cb)));
    }
    Kind getKind() const override { return InvalidKind; }
    Width getWidth() const override { return InvalidWidth; }
    unsigned getNumKids() const override { return 0; }
    klee::Expr* getKid(unsigned) const override { return nullptr; }
    klee::Expr* rebuild(klee::Expr*[]) const override { return nullptr; }
    void operator()() const { m_callback(); }
private:
    explicit LambdaExpr(CallbackType cb) : m_callback(std::move(cb)) {}
    CallbackType m_callback;
};

} // namespace kubu

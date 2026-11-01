// Minimal self-registering test framework (no external dependencies).
#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace t {

struct Case {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<Case>& cases() {
    static std::vector<Case> c;
    return c;
}
inline int& failures() {
    static int f = 0;
    return f;
}
inline int& checks() {
    static int c = 0;
    return c;
}

struct Reg {
    Reg(const char* n, std::function<void()> f) { cases().push_back({n, std::move(f)}); }
};

}  // namespace t

#define TEST_CAT2(a, b) a##b
#define TEST_CAT(a, b) TEST_CAT2(a, b)
#define TEST(name)                                                   \
    static void TEST_CAT(test_, name)();                             \
    static ::t::Reg TEST_CAT(reg_, name)(#name, &TEST_CAT(test_, name)); \
    static void TEST_CAT(test_, name)()

#define CHECK_EQ(a, b)                                                                                  \
    do {                                                                                                \
        ++::t::checks();                                                                                \
        const auto va_ = (a);                                                                           \
        const auto vb_ = (b);                                                                           \
        if (!(va_ == vb_)) {                                                                            \
            ++::t::failures();                                                                          \
            std::printf("  FAIL %s:%d: %s == %s\n    got 0x%llX, expected 0x%llX\n", __FILE__, __LINE__, #a, #b, \
                        (unsigned long long)(va_), (unsigned long long)(vb_));                          \
        }                                                                                               \
    } while (0)

#define CHECK_STR(a, b)                                                                              \
    do {                                                                                             \
        ++::t::checks();                                                                             \
        const std::string va_ = (a);                                                                 \
        const std::string vb_ = (b);                                                                 \
        if (va_ != vb_) {                                                                            \
            ++::t::failures();                                                                       \
            std::printf("  FAIL %s:%d: %s\n    got \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #a, \
                        va_.c_str(), vb_.c_str());                                                   \
        }                                                                                            \
    } while (0)

#define CHECK(c)                                                                \
    do {                                                                        \
        ++::t::checks();                                                        \
        if (!(c)) {                                                             \
            ++::t::failures();                                                  \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);          \
        }                                                                       \
    } while (0)

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <exception>

namespace test_framework {

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& get_registry() {
    static std::vector<TestCase> registry;
    return registry;
}

inline void register_test(const std::string& suite, const std::string& name, std::function<void()> func) {
    get_registry().push_back({suite, name, std::move(func)});
}

#define TEST(suite, name) \
    void test_##suite##_##name(); \
    namespace { \
        struct Reg_##suite##_##name { \
            Reg_##suite##_##name() { \
                ::test_framework::register_test(#suite, #name, test_##suite##_##name); \
            } \
        } reg_##suite##_##name; \
    } \
    void test_##suite##_##name()

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error("Assertion failed: " #cond " at " __FILE__ ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            throw std::runtime_error("Assertion failed: NOT " #cond " at " __FILE__ ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            throw std::runtime_error("Assertion failed: (" #a " == " #b ") at " __FILE__ ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            throw std::runtime_error("Assertion failed: (" #a " != " #b ") at " __FILE__ ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

inline int run_all_tests() {
    size_t passed = 0;
    size_t failed = 0;
    std::cout << "\n==========================================" << std::endl;
    std::cout << " Running AdaptiveDS Automated Test Suite " << std::endl;
    std::cout << "==========================================\n" << std::endl;

    for (const auto& tc : get_registry()) {
        std::cout << "[RUN     ] " << tc.suite << "." << tc.name << std::flush;
        try {
            tc.func();
            std::cout << " -> [PASS]" << std::endl;
            ++passed;
        } catch (const std::exception& e) {
            std::cout << " -> [FAIL]\n    Error: " << e.what() << std::endl;
            ++failed;
        } catch (...) {
            std::cout << " -> [FAIL]\n    Unknown exception thrown" << std::endl;
            ++failed;
        }
    }

    std::cout << "\n------------------------------------------" << std::endl;
    std::cout << "Test Summary: " << passed << " Passed, " << failed << " Failed, " 
              << (passed + failed) << " Total" << std::endl;
    std::cout << "------------------------------------------\n" << std::endl;

    return failed == 0 ? 0 : 1;
}

} // namespace test_framework

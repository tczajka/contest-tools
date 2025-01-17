#include "reader.h"
#include <cassert>
#include <iostream>
#include <sstream>

template <typename F>
void assert_error(F f, unsigned long long line, unsigned long long column) {
    try {
        f();
        assert(false);
    } catch (const Reader::Error &e) {
        assert(e.line == line);
        assert(e.column == column);
    }
}

void test_read_chars_strict() {
    std::istringstream input("a b\n");
    Reader reader(input, Reader::Strictness::strict);

    assert(reader.peek() == 'a');
    assert(reader.read_char() == 'a');
    reader.read_space();
    assert(reader.read_char() == 'b');
    reader.read_eoln();
}

void test_read_chars_permissive() {
    std::istringstream input("a    b   \t  \n c \n ");
    Reader reader(input, Reader::Strictness::permissive);

    assert(reader.peek() == 'a');
    assert(reader.read_char() == 'a');
    reader.read_space();
    assert(reader.read_char() == 'b');
    reader.read_eoln();
    reader.read_space();
    assert(reader.read_char() == 'c');
}

void test_missing_space_strict() {
    std::istringstream input("ab");
    Reader reader(
            input,
            Reader::Strictness::strict,
            Reader::ErrorHandling::exception);

    assert(reader.read_char() == 'a');
    assert_error([&] { reader.read_space(); }, 1, 2);
}

void test_missing_space_permissive() {
    std::istringstream input("ab");
    Reader reader(
            input,
            Reader::Strictness::permissive,
            Reader::ErrorHandling::exception);

    assert(reader.read_char() == 'a');
    assert_error([&] { reader.read_space(); }, 1, 2);
}

void test_missing_eoln_strict() {
    std::istringstream input("a");
    Reader reader(
            input,
            Reader::Strictness::strict,
            Reader::ErrorHandling::exception);

    assert(reader.read_char() == 'a');
    assert_error([&] { reader.read_eoln(); }, 1, 2);
}

void test_missing_eoln_permissive() {
    std::istringstream input("a   ");
    Reader reader(input, Reader::Strictness::permissive);

    assert(reader.read_char() == 'a');
    reader.read_eoln();
}

void test_read_line() {
    std::istringstream input("ab cd\n");
    Reader reader(input, Reader::Strictness::strict);

    assert(reader.read_line() == "ab cd");
}

void test_read_strings_strict() {
    std::istringstream input("ab cd ef\n");
    Reader reader(input, Reader::Strictness::strict);

    assert((reader.read_strings(3) == std::vector<std::string>{"ab", "cd", "ef"}));
    reader.read_eoln();
}

void test_read_strings_permissive() {
    std::istringstream input("  ab   cd   ef \t");
    Reader reader(input, Reader::Strictness::permissive);

    assert((reader.read_strings(3) == std::vector<std::string>{"ab", "cd", "ef"}));
    reader.read_eoln();
}

void test_read_strings_fail_strict() {
    std::istringstream input("ab cd  ef\n");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);

    assert_error([&] { reader.read_strings(3); }, 1, 7);
}

void test_read_strings_fail_permissive() {
    std::istringstream input(" ab  cd  \nef\n");
    Reader reader(input, Reader::Strictness::permissive, Reader::ErrorHandling::exception);

    assert_error([&] { reader.read_strings(3); }, 1, 10);
}

void test_read_ints_strict() {
    std::istringstream input("3 -100\n");
    Reader reader(input, Reader::Strictness::strict);

    assert((reader.read_ints(2, -100, 100) == std::vector<int>{3, -100}));
    reader.read_eoln();
}

void test_read_ints_permissive() {
    std::istringstream input("  003 -0100\n");
    Reader reader(input, Reader::Strictness::permissive);

    assert((reader.read_ints(2, -100, 100) == std::vector<int>{3, -100}));
    reader.read_eoln();
}

void test_read_ints_strict_extra_space() {
    std::istringstream input("3  -100\n");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);

    assert_error([&] { reader.read_ints(2, -100, 100); }, 1, 3);
}

void test_read_int_strict_leading_zero() {
    std::istringstream input("03");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);

    assert_error([&] { reader.read_int(-100, 100); }, 1, 3);
}

void test_read_int_strict_negative_zero() {
    std::istringstream input("-0");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);

    assert_error([&] { reader.read_int(-100, 100); }, 1, 3);
}

void test_read_int_out_of_range() {
    std::istringstream input("101");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);

    assert_error([&] { reader.read_int(-100, 100); }, 1, 4);
}

void test_read_unsigned_negative_zero() {
    std::istringstream input("-0");
    Reader reader(input, Reader::Strictness::permissive, Reader::ErrorHandling::exception);

    assert_error([&] { reader.read_int(0u, 100u); }, 1, 3);
}

void test_read_reals_strict() {
    std::istringstream input("3 -100.0 3.14\n");
    Reader reader(input, Reader::Strictness::strict);

    assert((reader.read_reals(3, -100.0, 100.0) == std::vector<double>{3, -100.0, 3.14}));
    reader.read_eoln();
}

void test_read_reals_permissive() {
    std::istringstream input("   3 -1e+2 0003.14\n");
    Reader reader(input, Reader::Strictness::permissive);

    assert((reader.read_reals(3, -100.0, 100.0) == std::vector<double>{3, -100.0, 3.14}));
    reader.read_eoln();
}

void test_read_real_strict_leading_zero() {
    std::istringstream input("013.13");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);
    assert_error([&] { reader.read_real(-100.0, 100.0); }, 1, 7);
}

void test_read_real_strict_negative_zero() {
    std::istringstream input("-0.000");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);
    assert_error([&] { reader.read_real(-100.0, 100.0); }, 1, 7);
}

void test_read_real_strict_too_much_precision() {
    std::istringstream input("13.000");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);
    assert_error([&] { reader.read_real(-100.0, 100.0, 2); }, 1, 7);
}

void test_read_real_strict_scientific() {
    std::istringstream input("1e2");
    Reader reader(input, Reader::Strictness::strict, Reader::ErrorHandling::exception);
    assert_error([&] { reader.read_real(-100.0, 100.0); }, 1, 2);
}

void test_read_real_out_of_range() {
    std::istringstream input("100.13");
    Reader reader(input, Reader::Strictness::permissive, Reader::ErrorHandling::exception);
    assert_error([&] { reader.read_real(-100.0, 100.0); }, 1, 7);
}

int main() {
    test_read_chars_strict();
    test_read_chars_permissive();
    test_missing_space_strict();
    test_missing_space_permissive();
    test_missing_eoln_strict();
    test_missing_eoln_permissive();
    test_read_line();
    test_read_strings_strict();
    test_read_strings_permissive();
    test_read_strings_fail_strict();
    test_read_strings_fail_permissive();
    test_read_ints_strict();
    test_read_ints_permissive();
    test_read_ints_strict_extra_space();
    test_read_int_strict_leading_zero();
    test_read_int_strict_negative_zero();
    test_read_int_out_of_range();
    test_read_unsigned_negative_zero();
    test_read_reals_strict();
    test_read_reals_permissive();
    test_read_real_strict_leading_zero();
    test_read_real_strict_negative_zero();
    test_read_real_strict_too_much_precision();
    test_read_real_strict_scientific();
    test_read_real_out_of_range();
    std::cout << "OK\n";
}

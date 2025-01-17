#include "reader.h"
#include <cassert>
#include <iostream>

void test_reader(
        const std::string_view file_name,
        const Reader::Strictness strictness) {
    Reader reader(file_name, strictness);

    assert(reader.peek() == 'a');
    assert(reader.read_char() == 'a');
    reader.read_space();
    assert(reader.read_char() == 'b');
    reader.read_eoln();

    assert(reader.read_line() == "abc def");

    assert((reader.read_strings(2) == std::vector<std::string>{"abc", "def"}));
    reader.read_eoln();
    assert((reader.read_ints(2, -100, 100) == std::vector<int>{-17, 42}));
    reader.read_eoln();
    assert((reader.read_reals(2, -100.0, 100.0) == std::vector<double>{-3.14, 3.14}));
    reader.read_eoln();
}

int main() {
    test_reader("tests/reader.strict", Reader::Strictness::strict);
    test_reader("tests/reader.permissive", Reader::Strictness::permissive);
    std::cout << "OK\n";
}

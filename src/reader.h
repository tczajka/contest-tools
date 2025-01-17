#ifndef READER_H
#define READER_H

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

class Reader {
public:
    // Use `strict` for input verifiers.
    // Use `permissive` for output checkers.
    enum class Strictness {
        // Allow extra whitespace, leading zeros, etc.
        permissive,
        // Disallow extra whitespace, leading zeros, etc.
        strict,
    };

    // Create a Reader.
    explicit Reader(std::string_view file_name,
                    Strictness strictness = Strictness::strict);

    // Reader can't be copied.
    Reader(const Reader &) = delete;
    void operator=(const Reader &) = delete;
    Reader(Reader &&) = default;
    Reader &operator=(Reader &&) = default;

    // Calls read_eof.
    ~Reader();

    // Prints error to stdout and exits the process with exit code 1.
    void error(std::string_view error) const;

    // Look at the next character but do not consume it.
    char peek() const;

    // Read a character, including whitespace.
    char read_char();

    // Read a single space.
    // In permissive mode allows any nonempty whitespace.
    void read_space();

    // Read a single eoln character.
    // In permissive mode skips whitespace.
    // In permissive mode no error on eof.
    void read_eoln();

    // Verify we have reached end of file.
    // In permissive mode skips any remaining whitespace.
    void read_eof();

    // Endline-terminated string.
    // Does not skip whitespace.
    std::string read_line();

    // Non-empty whitespace-terminated string.
    // In permissive mode skips leading whitespace (but not eoln).
    std::string read_string();

    // Works for (unsigned) int, long, long long.
    // In permissive mode skips leading whitespace (but not eoln).
    // In strict mode leading zeros and "-0" not allowed.
    template <typename T>
    T read_int(T min, T max);

    // Works for float, double, long double.
    // Format: [0-9]+(.[0-9]*).
    // In permissive mode skips leading whitespace (but not eoln).
    // In strict mode leading zeros and "-0" not allowed.
    template <typename T>
    T read_real(
            T min,
            T max,
            std::size_t max_fractional_digits = std::numeric_limits<std::size_t>::max());

    // Whitespace-separated strings in a single line.
    // In strict mode separated by single spaces.
    // In permissive mode separated by any whitespace.
    std::vector<std::string> read_strings(std::size_t n);

    // Whitespace-separated integers in a single line.
    // In strict mode separated by single spaces.
    // In permissive mode separated by any whitespace.
    template <typename T>
    std::vector<T> read_ints(std::size_t n, T min, T max);

    // Whitespace-separated reals in a single line.
    // In strict mode separated by single spaces.
    // In permissive mode separated by any whitespace.
    template <typename T>
    std::vector<T> read_reals(
            std::size_t n,
            T min,
            T max,
            std::size_t max_fractional_digits = std::numeric_limits<std::size_t>::max());

private:
    void advance_char();
    void skip_whitespace_in_line(bool required = false);

    std::ifstream m_input;
    Strictness m_strictness;

    bool m_eof = false;
    char m_next_char = 0;

    unsigned long long m_line = 1;
    unsigned long long m_column = 0;
};

inline Reader::Reader(const std::string_view file_name, const Strictness strictness):
    m_input(std::string(file_name)),
    m_strictness(strictness)
{
    if (m_input.fail()) {
        error(std::string("can't open file ") + std::string(file_name));
    }
    advance_char();
}

inline Reader::~Reader() {
    read_eof();
}

inline void Reader::error(const std::string_view error) const {
    std::cout << "ERROR(" << m_line << ":" << m_column << "): " << error << "\n";
    std::exit(1);
}

inline char Reader::peek() const {
    if (m_eof) {
        error("Unexpected EOF");
    }
    return m_next_char;
}

inline char Reader::read_char() {
    const char res = peek();
    advance_char();
    return res;
}

inline void Reader::read_space() {
    if (m_strictness == Strictness::strict) {
        if (m_next_char != ' ') {
            error("Expected space");
        }
        advance_char();
    } else {
        skip_whitespace_in_line(true);
    }
}

inline void Reader::read_eoln() {
    if (m_strictness == Strictness::permissive) {
        skip_whitespace_in_line();
        if (m_eof) return;
    }
    if (m_next_char != '\n') {
        error("Expected EOLN");
    }
    advance_char();
}

inline void Reader::read_eof() {
    if (m_strictness == Strictness::permissive) {
        while (std::isspace(m_next_char)) {
            advance_char();
        }
    }
    if (!m_eof) {
        error("Expected EOF");
    }
}

inline std::string Reader::read_line() {
    std::string res;
    while (!m_eof && m_next_char != '\n') {
        res += m_next_char;
        advance_char();
    }
    if (m_eof) {
        if (m_strictness == Strictness::strict) {
            error("Unexpected EOF");
        }
    } else {
        advance_char();
    }
    return res;
}

inline std::string Reader::read_string() {
    if (m_strictness == Strictness::permissive) {
        skip_whitespace_in_line();
    }
    std::string res;
    while (!m_eof && !std::isspace(m_next_char)) {
        res += m_next_char;
        advance_char();
    }
    if (res.empty()) {
        error("Expected string");
    }
    return res;
}

template <typename T>
inline T Reader::read_int(const T min, const T max) {
    if (m_strictness == Strictness::permissive) {
        skip_whitespace_in_line();
    }
    std::string s;
    if (m_next_char == '-') {
        s += m_next_char;
        advance_char();
    }
    while (std::isdigit(m_next_char)) {
        s += m_next_char;
        advance_char();
    }
    if (m_strictness == Strictness::strict) {
        if ((s.size() >= 2 && s[0] == '0') ||
            (s.size() >= 3 && s[0] == '-' && s[1] == '0')) {
            error("Leading 0");
        }
        if (s == "-0") {
            error("Negative 0");
        }
    }
    const char *const begin = s.data();
    const char *const end = s.data() + s.size();
    T res;
    const auto code = std::from_chars(begin, end, res);
    if (code.ec != std::errc{} || code.ptr != end || res < min || res > max) {
        error(std::string("Expected integer in range [")
                + std::to_string(min) + ", "
                + std::to_string(max) + "]");
    }
    return res;
}

template <typename T>
inline T Reader::read_real(
        const T min,
        const T max,
        const std::size_t max_fractional_digits) {
    if (m_strictness == Strictness::permissive) {
        skip_whitespace_in_line();
    }
    std::string s;
    if (m_next_char == '-') {
        s += m_next_char;
        advance_char();
    }
    while (std::isdigit(m_next_char)) {
        s += m_next_char;
        advance_char();
    }
    if (m_next_char == '.') {
        s += m_next_char;
        advance_char();
        std::size_t fractional_digits = 0;
        while (std::isdigit(m_next_char)) {
            s += m_next_char;
            ++fractional_digits;
            advance_char();
        }
        if (fractional_digits > max_fractional_digits) {
            error(std::string("More than ") + std::to_string(max_fractional_digits) +
                    " fractional_digits");
        }
    }
    if (m_strictness == Strictness::strict) {
        if ((s.size() >= 2 && s[0] == '0' && s[1] != '.') ||
            (s.size() >= 3 && s[0] == '-' && s[1] == '0' && s[2] != '.')) {
            error("Leading 0");
        }
    }
    const char *const begin = s.data();
    const char *const end = s.data() + s.size();
    T res;
    const auto code = std::from_chars(begin, end, res, std::chars_format::fixed);
    if (code.ec != std::errc{} || code.ptr != end || res < min || res > max) {
        error(std::string("Expected real in range [")
                + std::to_string(min) + ", "
                + std::to_string(max) + "]");
    }
    if (m_strictness == Strictness::strict && res == 0.0 && s.size() >= 1 && s[0] == '-') {
        error("Negative 0");
    }
    return res;
}

inline std::vector<std::string> Reader::read_strings(const std::size_t n) {
    std::vector<std::string> res;
    for (std::size_t i = 0; i != n; ++i) {
        if (i != 0) read_space();
        res.push_back(read_string());
    }
    return res;
}

template <typename T>
inline std::vector<T> Reader::read_ints(const std::size_t n, const T min, const T max) {
    std::vector<T> res;
    for (std::size_t i = 0; i != n; ++i) {
        if (i != 0) read_space();
        res.push_back(read_int(min, max));
    }
    return res;
}

template <typename T>
inline std::vector<T> Reader::read_reals(
        const std::size_t n,
        const T min,
        const T max,
        const std::size_t max_fractional_digits) {
    std::vector<T> res;
    for (std::size_t i = 0; i != n; ++i) {
        if (i != 0) read_space();
        res.push_back(read_real(min, max, max_fractional_digits));
    }
    return res;
}

inline void Reader::advance_char() {
    if (m_eof) {
        throw std::logic_error("Reader::advance_char beyond EOF");
    }
    if (m_next_char == '\n') {
        ++m_line;
        m_column = 1;
    } else {
        ++m_column;
    }
    const auto c = m_input.get();
    if (m_input.bad()) {
        error("read failed");
    }
    if (c == std::ifstream::traits_type::eof()) {
        m_eof = true;
        m_next_char = 0;
    } else {
        m_next_char = std::ifstream::traits_type::to_char_type(c);
    }
}

inline void Reader::skip_whitespace_in_line(const bool required) {
    bool skipped = false;
    while (std::isspace(m_next_char) && m_next_char != '\n') {
        skipped = true;
        advance_char();
    }
    if (required && !skipped) {
        error("Expected whitespace");
    }
}

#endif

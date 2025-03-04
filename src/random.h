// Random number generator.
//
// Cryptographically strong. Uses ChaCha20.

#ifndef RANDOM_H
#define RANDOM_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace random_private {

using std::array, std::uint8_t, std::uint32_t, std::uint64_t, std::int64_t;

// Cryptographic key.
//
// WARNING: This key should be random, unique per contest and kept private!
//
// Linux command to generate:
// hexdump -e '4/4 "0x%08X, " "\n"' /dev/random | head -n 2
constexpr array<uint32_t, 8> key = {
    0xD2EE7398, 0xC1963D5C, 0xAA54D7C8, 0x5DA5A588,
    0x7391688F, 0x3BE114E4, 0x07DFCCA9, 0x5053BCBC,
};

// A random generator.
class Random {
public:
    // Random stream is determined by (key, problem_name, test_id).
    //
    // problem_name.size() <= 4
    explicit Random(const std::string_view problem_name,
                    uint32_t test_id);

    // Move only.
    Random(const Random &) = delete;
    void operator=(const Random &) = delete;
    Random(Random &&) = default;
    Random &operator=(Random &&) = default;

    uint64_t bits(int n);

    int uniform_int(int min, int max);
    unsigned uniform_uint(unsigned min, unsigned max);
    int64_t uniform_int64(int64_t min, int64_t max);
    uint64_t uniform_uint64(uint64_t min, uint64_t max);

    template <typename Iter>
    void shuffle(Iter begin, Iter end);

private:
    uint64_t m_nonce;
    uint64_t m_counter = 0;
    array<uint32_t, 16> m_word_buffer = {};
    int m_word_buffer_next = 16;

    uint32_t m_bits_buffer = 0;
    int m_num_bits = 0;

    uint64_t m_number_buffer = 0;
    uint64_t m_number_range = 1;
};

template <int bits>
inline uint32_t rotate_left(const uint32_t x) {
    static_assert(bits > 0 && bits < 32);
    return (x << bits) | (x >> (32 - bits));
}

inline void quarter_round(
        uint32_t &a,
        uint32_t &b,
        uint32_t &c,
        uint32_t &d)
{
    a += b;
    d ^= a;
    d = rotate_left<16>(d);
    c += d;
    b ^= c;
    b = rotate_left<12>(b);
    a += b;
    d ^= a;
    d = rotate_left<8>(d);
    c += d;
    b ^= c;
    b = rotate_left<7>(b);
}

template<int rounds>
array<uint32_t, 16> chacha(
        const array<uint32_t, 8> &key,
        const uint64_t nonce,
        const uint64_t counter)
{
    static_assert(rounds == 8 || rounds == 12 || rounds == 20);

    array<uint32_t, 16> input;
    input[0] = 0x61707865; // "expa"
    input[1] = 0x3320646e; // "nd 3"
    input[2] = 0x79622d32; // "2-by"
    input[3] = 0x6b206574; // "te k'
    std::copy(key.begin(), key.end(), input.begin() + 4);
    input[12] = static_cast<uint32_t>(counter);
    input[13] = static_cast<uint32_t>(counter >> 32);
    input[14] = static_cast<uint32_t>(nonce);
    input[15] = static_cast<uint32_t>(nonce >> 32);

    array<uint32_t, 16> x = input;

    for (int double_round = 0; double_round < rounds / 2; ++double_round) {
        quarter_round(x[0], x[4], x[8], x[12]);
        quarter_round(x[1], x[5], x[9], x[13]);
        quarter_round(x[2], x[6], x[10], x[14]);
        quarter_round(x[3], x[7], x[11], x[15]);

        quarter_round(x[0], x[5], x[10], x[15]);
        quarter_round(x[1], x[6], x[11], x[12]);
        quarter_round(x[2], x[7], x[8], x[13]);
        quarter_round(x[3], x[4], x[9], x[14]);
    }

    for (int i = 0; i < 16; ++i) {
        x[i] += input[i];
    }

    return x;
}

inline Random::Random(
        const std::string_view problem_name,
        const uint32_t test_id)
{
    if (problem_name.size() > 4) {
        throw std::invalid_argument("problem_name too long");
    }
    m_nonce = test_id;
    for (size_t i=0; i != problem_name.size(); ++i) {
        const uint8_t byte = static_cast<uint8_t>(problem_name[i]);
        if (byte == 0) {
            throw std::invalid_argument("0 bytes in problem_name");
        }
        m_nonce |= static_cast<uint64_t>(byte) << (4 + i);
    }
}

inline uint64_t Random::bits(int n) {
    if (n > 64) {
        throw std::invalid_argument("n > 64");
    }
    uint64_t result = 0;
    while (n >= m_num_bits) {
        result <<= m_num_bits;
        result |= m_bits_buffer;
        n -= m_num_bits;

        // refill bits
        if (m_word_buffer_next == 16) {
            // refill words
            m_word_buffer = chacha<20>(key, m_nonce, m_counter++);
            m_word_buffer_next = 0;
            if (m_counter == 0) {
                throw std::runtime_error("Random counter overflow");
            }
            m_word_buffer_next = 0;
        }
        m_bits_buffer = m_word_buffer[m_word_buffer_next++];
        m_num_bits = 32;
    }
    // n < m_num_bits <= 32
    result <<= n;
    result |= m_bits_buffer & ((1u << n) - 1u);
    m_bits_buffer >>= n;
    m_num_bits -= n;
    return result;
}

inline int Random::uniform_int(const int min, const int max) {
    return static_cast<int>(uniform_int64(min, max));
}

inline unsigned Random::uniform_uint(const unsigned min, const unsigned max) {
    return static_cast<unsigned>(uniform_uint64(min, max));
}

inline int64_t Random::uniform_int64(const int64_t min, const int64_t max) {
    if (min > max) {
        throw std::invalid_argument("min > max");
    }
    const uint64_t umin = static_cast<uint64_t>(min);
    const uint64_t umax = static_cast<uint64_t>(max);
    return static_cast<int64_t>(uniform_uint64(0, umax - umin) + umin);
}

inline uint64_t Random::uniform_uint64(const uint64_t min, const uint64_t max) {
    if (min > max) {
        throw std::invalid_argument("min > max");
    }
    if (min == 0u && max == std::numeric_limits<uint64_t>::max()) {
        return bits(64);
    }
    const uint64_t n = max - min + 1u;

    for (;;) {
        // refill number buffer
        const int zeros = __builtin_clzll(m_number_range);
        m_number_range <<= zeros;
        m_number_buffer <<= zeros;
        m_number_buffer |= bits(zeros);

        const uint64_t num_groups = m_number_range / n;
        const uint64_t small_group = m_number_range % n;
        const uint64_t group = m_number_buffer / n;
        const uint64_t in_group = m_number_buffer % n;
        if (group < num_groups) {
            m_number_range = num_groups;
            m_number_buffer = group;
            return min + in_group;
        } else {
            m_number_range = small_group;
            m_number_buffer = in_group;
        }
    }
}

template <typename Iter>
void Random::shuffle(Iter begin, Iter end) {
    const uint64_t n = end - begin;
    for (uint64_t i=1; i < n; ++i) {
        const uint64_t j = uniform_uint64(0, i);
        std::swap(*(begin+i), *(begin+j));
    }
}

} // namespace random

using random_private::Random;

#endif

// Random number generator.
//
// Cryptographically strong. Uses ChaCha20.

#ifndef RANDOM_H
#define RANDOM_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace random_private {

using std::array, std::uint8_t, std::uint32_t, std::uint64_t;

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
// Can be used by C++ standard library functions.
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

    // Standard library API.
    using result_type = uint32_t;
    static constexpr uint32_t min() { return 0; }
    static constexpr uint32_t max() { return 0xffffffff; }
    uint32_t operator()();

    // Helper: uniformly random integer in [min, max] inclusive.
    template <typename T>
    T uniform_int(T min, T max);

private:
    uint64_t nonce;
    uint64_t counter = 0;
    array<uint32_t, 16> buffer = {};
    int buffer_next = 16;
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

Random::Random(
        const std::string_view problem_name,
        const uint32_t test_id)
{
    if (problem_name.size() > 4) {
        throw std::invalid_argument("problem_name too long");
    }
    nonce = test_id;
    for (size_t i=0; i != problem_name.size(); ++i) {
        const uint8_t byte = static_cast<uint8_t>(problem_name[i]);
        if (byte == 0) {
            throw std::invalid_argument("0 bytes in problem_name");
        }
        nonce |= static_cast<uint64_t>(byte) << (4 + i);
    }
}

uint32_t Random::operator()() {
    if (buffer_next == 16) {
        buffer = chacha<20>(key, nonce, counter);
        buffer_next = 0;
        ++counter;
        if (counter == 0) {
            throw std::runtime_error("Random counter overflow");
        }
    }
    return buffer[buffer_next++];
}

template <typename T>
T Random::uniform_int(const T min, const T max) {
    std::uniform_int_distribution<T> dist{min, max};
    return dist(*this);
}

} // namespace random

using random_private::Random;

#endif

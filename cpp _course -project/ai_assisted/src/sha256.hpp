#pragma once
#include "domain.hpp"
namespace sg {
// Self-contained SHA-256, FIPS 180-4 compression function. This teaching application
// uses a per-account random salt, not a production password KDF.
inline string sha256(const string &input) {
    static const std::uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2};
    std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::vector<std::uint8_t> d(input.begin(), input.end());
    std::uint64_t bits = std::uint64_t(d.size()) * 8;
    d.push_back(0x80);
    while (d.size() % 64 != 56)
        d.push_back(0);
    for (int j = 7; j >= 0; --j)
        d.push_back(std::uint8_t(bits >> (j * 8)));
    auto rr = [](std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
    for (size_t b = 0; b < d.size(); b += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (std::uint32_t(d[b + 4 * i]) << 24) | (std::uint32_t(d[b + 4 * i + 1]) << 16) |
                   (std::uint32_t(d[b + 4 * i + 2]) << 8) | d[b + 4 * i + 3];
        for (int i = 16; i < 64; ++i) {
            auto x = w[i - 15], y = w[i - 2];
            w[i] = w[i - 16] + (rr(x, 7) ^ rr(x, 18) ^ (x >> 3)) + w[i - 7] +
                   (rr(y, 17) ^ rr(y, 19) ^ (y >> 10));
        }
        std::uint32_t a = h[0], c = h[2], bb = h[1], dd = h[3], e = h[4], f = h[5], g = h[6],
                      hh = h[7];
        for (int i = 0; i < 64; ++i) {
            std::uint32_t t1 =
                hh + (rr(e, 6) ^ rr(e, 11) ^ rr(e, 25)) + ((e & f) ^ ((~e) & g)) + k[i] + w[i];
            std::uint32_t t2 = (rr(a, 2) ^ rr(a, 13) ^ rr(a, 22)) + ((a & bb) ^ (a & c) ^ (bb & c));
            hh = g;
            g = f;
            f = e;
            e = dd + t1;
            dd = c;
            c = bb;
            bb = a;
            a = t1 + t2;
        }
        h[0] += a;
        h[1] += bb;
        h[2] += c;
        h[3] += dd;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }
    std::ostringstream o;
    for (auto n : h)
        o << std::hex << std::setw(8) << std::setfill('0') << n;
    return o.str();
}
inline string randomSalt() {
    std::random_device r;
    std::ostringstream o;
    for (int i = 0; i < 4; ++i)
        o << std::hex << std::setw(8) << std::setfill('0') << r();
    return o.str();
}
inline void setPassword(Account &a, const string &p) {
    require(p.size() >= 6 && p.size() <= 100 && clean(p), "密码长度必须为 6—100，禁止控制字符");
    a.salt = randomSalt();
    a.hash = sha256(a.salt + ":" + p);
}
inline bool passwordIs(const Account &a, const string &p) {
    return a.hash == sha256(a.salt + ":" + p);
}
} // namespace sg

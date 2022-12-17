#pragma once

#include <algorithm>
#include <array>
#if defined(_M_ARM64) || defined(__aarch64__)
#define OOZ_ARCH_ARM64 1
#else
#define OOZ_ARCH_AMD64 1
#endif

#if OOZ_ARCH_ARM64

using __m128i = std::array<uint8_t, 16>;
using __m128 = std::array<uint8_t, 16>;
using __m64 = std::array<uint8_t, 8>;

inline auto _mm_cvtsi32_si128(int a) -> __m128i {
    __m128i dst{};
    memcpy(dst.data(), &a, 4);
    return dst;
}

inline auto _mm_unpacklo_epi8(__m128i const &src1, __m128i const &src2) -> __m128i {
    __m128i dst{};
    auto *p = dst.data();
    for (int i = 0; i < 8; ++i) {
        *p++ = src1[i];
        *p++ = src2[i];
    }
    return dst;
}

inline auto _mm_unpackhi_epi8(__m128i const &src1, __m128i const &src2) -> __m128i {
    __m128i dst{};
    auto *p = dst.data();
    for (int i = 0; i < 8; ++i) {
        *p++ = src1[8 + i];
        *p++ = src2[8 + i];
    }
    return dst;
}

inline auto _mm_unpacklo_epi16(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    auto *p = dst.data();
    for (int i = 0; i < 4; ++i) {
        *p++ = a[2 * i];
        *p++ = a[2 * i + 1];
        *p++ = b[2 * i];
        *p++ = b[2 * i + 1];
    }
    return dst;
}

inline auto _mm_packs_epi16(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    for (int i = 0; i < 8; ++i) {
        int16_t tmp;
        memcpy(&tmp, a.data() + 2 * i, 2);
        dst[i] = (int8_t)std::min<int16_t>(127, std::max<int16_t>(-128, tmp));
        memcpy(&tmp, b.data() + 2 * i, 2);
        dst[8 + i] = (int8_t)std::min<int16_t>(127, std::max<int16_t>(-128, tmp));
    }
    return dst;
}

inline auto _mm_loadl_epi64(void const *mem_addr) -> __m128i {
    __m128i dst{};
    memcpy(dst.data(), mem_addr, 8);
    return dst;
}

inline auto _mm_storel_epi64(void *mem_addr, __m128i const &a) { memcpy(mem_addr, a.data(), 8); }
inline auto _mm_storeh_pi(void *mem_addr, __m128 const &a) { memcpy(mem_addr, a.data() + 8, 8); }
inline auto _mm_castsi128_ps(__m128i const &a) -> __m128 {
    __m128 dst;
    memcpy(dst.data(), a.data(), 16);
    return dst;
}

inline auto _mm_loadu_si128(void const *mem_addr) -> __m128i {
    __m128i dst;
    memcpy(dst.data(), mem_addr, 16);
    return dst;
}

inline auto _mm_storeu_si128(void *mem_addr, __m128i const &a) { memcpy(mem_addr, a.data(), 16); }

inline auto _mm_set_epi16(short e7, short e6, short e5, short e4, short e3, short e2, short e1, short e0) -> __m128i {
    __m128i dst;
    memcpy(dst.data() + 0, &e0, 2);
    memcpy(dst.data() + 2, &e1, 2);
    memcpy(dst.data() + 4, &e2, 2);
    memcpy(dst.data() + 6, &e3, 2);
    memcpy(dst.data() + 8, &e4, 2);
    memcpy(dst.data() + 10, &e5, 2);
    memcpy(dst.data() + 12, &e6, 2);
    memcpy(dst.data() + 14, &e7, 2);
    return dst;
}

inline auto _mm_set1_epi8(char a) -> __m128i {
    __m128i dst;
    for (int i = 0; i < 16; ++i) {
        memcpy(dst.data() + i, &a, 1);
    }
    return dst;
}

inline auto _mm_set1_epi16(short a) -> __m128i {
    __m128i dst;
    for (int i = 0; i < 8; ++i) {
        memcpy(dst.data() + i * 2, &a, 2);
    }
    return dst;
}

inline auto _mm_set1_epi32(int a) -> __m128i {
    __m128i dst;
    for (int i = 0; i < 4; ++i) {
        memcpy(dst.data() + i * 4, &a, 4);
    }
    return dst;
}

inline auto _mm_shuffle_epi32(__m128i const &a, int imm8) -> __m128i {
    __m128i dst;
    for (int i = 0; i < 4; ++i) {
        memcpy(dst.data() + 4 * i, a.data() + 4 * (imm8 & 0b11), 4);
    }
    return dst;
}

inline auto _mm_add_epi8(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 16; ++i) {
        dst[i] = a[i] + b[i];
    }
    return dst;
}

inline auto _mm_add_epi16(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 8; ++i) {
        int16_t tmp_a, tmp_b;
        memcpy(&tmp_a, a.data() + i * 2, 2);
        memcpy(&tmp_b, b.data() + i * 2, 2);
        int16_t tmp = tmp_a + tmp_b;
        memcpy(dst.data() + i * 2, &tmp, 2);
    }
    return dst;
}

inline auto _mm_sub_epi8(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 16; ++i) {
        dst[i] = a[i] - b[i];
    }
    return dst;
}

inline auto _mm_sub_epi16(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 8; ++i) {
        int16_t tmp_a, tmp_b;
        memcpy(&tmp_a, a.data() + i * 2, 2);
        memcpy(&tmp_b, b.data() + i * 2, 2);
        int16_t tmp = tmp_a - tmp_b;
        memcpy(dst.data() + i * 2, &tmp, 2);
    }
    return dst;
}

inline auto _mm_max_epu8(__m128i a, __m128i b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 16; ++i) {
        dst[i] = b[i] > a[i] ? b[i] : a[i];
    }
    return dst;
}

inline auto _mm_cmpeq_epi8(__m128i a, __m128i b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 16; ++i) {
        dst[i] = (a[i] == b[i]) ? 0xFFu : 0;
    }
    return dst;
}

inline auto _mm_cmpgt_epi16(__m128i a, __m128i b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 8; ++i) {
        short tmp_a, tmp_b;
        memcpy(&tmp_a, a.data() + i * 2, 2);
        memcpy(&tmp_b, b.data() + i * 2, 2);
        short res = (tmp_a > tmp_b) ? 0xFFFF : 0;
        memcpy(dst.data() + i * 2, &res, 2);
    }
    return dst;
}

inline auto _mm_movemask_epi8(__m128i a) -> int {
    int ret{};
    for (size_t i = 0; i < 16; ++i) {
        if (a[i] & 0x80) {
            ret |= (1 << i);
        }
    }
    return ret;
}

inline auto _mm_srai_epi16(__m128i a, int imm8) -> __m128i {
    __m128i dst{};
    if (imm8 <= 15) {
        for (int i = 0; i < 7; ++i) {
            int16_t tmp;
            memcpy(&tmp, a.data() + i * 2, 2);
            tmp >>= imm8;
            memcpy(dst.data() + i * 2, &tmp, 2);
        }
    }
    return dst;
}

inline auto _mm_srli_epi16(__m128i a, int imm8) -> __m128i {
    __m128i dst{};
    if (imm8 <= 15) {
        for (int i = 0; i < 7; ++i) {
            uint16_t tmp;
            memcpy(&tmp, a.data() + i * 2, 2);
            tmp >>= imm8;
            memcpy(dst.data() + i * 2, &tmp, 2);
        }
    }
    return dst;
}

inline auto _mm_slli_si128(__m128i const &a, int imm8) -> __m128i {
    __m128i dst{};
    if (imm8 == 0) {
        dst = a;
    } else if (imm8 <= 15) {
        int shift = imm8 * 8;
        int unshift = 64 - shift;
        uint64_t lo{}, hi{};
        uint64_t mask = (1ull << shift) - 1;
        memcpy(&lo, a.data(), 8);
        memcpy(&hi, a.data() + 8, 8);
        hi <<= shift;
        hi |= (lo >> unshift) & mask;
        lo <<= shift;
        memcpy(dst.data(), &lo, 8);
        memcpy(dst.data() + 8, &hi, 8);
    }
    return dst;
}

inline auto _mm_and_si128(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 16; ++i) {
        dst[i] = a[i] & b[i];
    }
    return dst;
}

inline auto _mm_xor_si128(__m128i const &a, __m128i const &b) -> __m128i {
    __m128i dst;
    for (size_t i = 0; i < 16; ++i) {
        dst[i] = a[i] ^ b[i];
    }
    return dst;
}

inline auto _mm_prefetch(char const *p, int i) {}

enum : int { _MM_HINT_T0 = 1 };

#endif

/*  Binary Image Basic Filters (x64 AVX2)
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_Kernels_BinaryImage_BasicFilters_x64_AVX2_H
#define PokemonAutomation_Kernels_BinaryImage_BasicFilters_x64_AVX2_H

#include "Kernels/PartialWordAccess/Kernels_PartialWordAccess_x64_AVX2.h"
#include "Kernels_BinaryImage_BasicFilters.h"

namespace PokemonAutomation{
namespace Kernels{


class MinRgbFilter_x64_AVX2{
public:
    MinRgbFilter_x64_AVX2(
        uint8_t min_alpha, uint8_t min_red, uint8_t min_green, uint8_t min_blue
    ){
        uint32_t smin =
            ((uint32_t)min_alpha << 24) |
            ((uint32_t)min_red << 16) |
            ((uint32_t)min_green << 8) |
            (uint32_t)min_blue;
        m_mins = _mm256_set1_epi32(smin ^ 0x80808080);
    }

    PA_FORCE_INLINE uint64_t convert64(const uint32_t* pixels) const{
        uint64_t bits = 0;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels +  0))) <<  0;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels +  8))) <<  8;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 16))) << 16;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 24))) << 24;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 32))) << 32;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 40))) << 40;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 48))) << 48;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 56))) << 56;
        return bits;
    }
    PA_FORCE_INLINE uint64_t convert64(const uint32_t* pixels, size_t count) const{
        uint64_t bits = 0;
        size_t c = 0;
        size_t lc = count / 8;
        do{
            __m256i pixel = _mm256_loadu_si256((const __m256i*)pixels);
            bits |= convert8(pixel) << c;
            pixels += 8;
            c += 8;
        }while (--lc);
        count %= 8;
        if (count){
            PartialWordLoader32_x64_AVX2 loader(count);
            __m256i pixel = loader.load(pixels);
            bits |= convert8(pixel) << c;
        }
        return bits;
    }

private:
    PA_FORCE_INLINE uint64_t convert8(__m256i pixel) const{
        pixel = _mm256_xor_si256(pixel, _mm256_set1_epi8((uint8_t)0x80));
        __m256i cmp = _mm256_cmpgt_epi8(m_mins, pixel);
        cmp = _mm256_cmpeq_epi32(cmp, _mm256_setzero_si256());
        return _mm256_movemask_ps(_mm256_castsi256_ps(cmp));
    }

private:
    __m256i m_mins;
};



class RgbRangeFilter_x64_AVX2{
public:
    RgbRangeFilter_x64_AVX2(
        uint8_t min_alpha, uint8_t max_alpha,
        uint8_t min_red, uint8_t max_red,
        uint8_t min_green, uint8_t max_green,
        uint8_t min_blue, uint8_t max_blue
    ){
        uint32_t smin =
            ((uint32_t)min_alpha << 24) |
            ((uint32_t)min_red << 16) |
            ((uint32_t)min_green << 8) |
            (uint32_t)min_blue;
        uint32_t smax =
            ((uint32_t)max_alpha << 24) |
            ((uint32_t)max_red << 16) |
            ((uint32_t)max_green << 8) |
            (uint32_t)max_blue;
        m_mins = _mm256_set1_epi32(smin ^ 0x80808080);
        m_maxs = _mm256_set1_epi32(smax ^ 0x80808080);
    }

    PA_FORCE_INLINE uint64_t convert64(const uint32_t* pixels) const{
        uint64_t bits = 0;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels +  0))) <<  0;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels +  8))) <<  8;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 16))) << 16;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 24))) << 24;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 32))) << 32;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 40))) << 40;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 48))) << 48;
        bits |= convert8(_mm256_loadu_si256((const __m256i*)(pixels + 56))) << 56;
        return bits;
    }
    PA_FORCE_INLINE uint64_t convert64(const uint32_t* pixels, size_t count) const{
        uint64_t bits = 0;
        size_t c = 0;
        size_t lc = count / 8;
        do{
            __m256i pixel = _mm256_loadu_si256((const __m256i*)pixels);
            bits |= convert8(pixel) << c;
            pixels += 8;
            c += 8;
        }while (--lc);
        count %= 8;
        if (count){
            PartialWordLoader32_x64_AVX2 loader(count);
            __m256i pixel = loader.load(pixels);
            bits |= convert8(pixel) << c;
        }
        return bits;
    }

private:
    PA_FORCE_INLINE uint64_t convert8(__m256i pixel) const{
        pixel = _mm256_xor_si256(pixel, _mm256_set1_epi8((uint8_t)0x80));
        __m256i cmp0 = _mm256_cmpgt_epi8(m_mins, pixel);
        __m256i cmp1 = _mm256_cmpgt_epi8(pixel, m_maxs);
        cmp0 = _mm256_or_si256(cmp0, cmp1);
        cmp0 = _mm256_cmpeq_epi32(cmp0, _mm256_setzero_si256());
        return _mm256_movemask_ps(_mm256_castsi256_ps(cmp0));
    }

private:
    __m256i m_mins;
    __m256i m_maxs;
};



}
}
#endif

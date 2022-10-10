#ifndef HQ2X_HPP
#define HQ2X_HPP

#include <array>

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include "common.hpp"

#define P(mask, des_res) ((diffs & (mask)) == (des_res))
#define WDIFF(c1, c2) yuvDifference(c1, c2)


constexpr uint8_t y_threshold = 0x30;
constexpr uint8_t u_threshold = 0x07;
constexpr uint8_t v_threshold = 0x06;

static inline glm::uvec3 rgbToYuv(glm::uvec3 val) {
    return {
        (0.299f * val.r) + (0.587f * val.g) + (0.114f * val.b),
        ((-0.169f * val.r) + (-0.331 * val.g) + (0.5f * val.b)) + 128,
        ((0.5f * val.r) + (-0.419f * val.g) + (-0.081f * val.b)) + 128};
}

static inline uint32_t rgbToYuv(uint32_t val) {
    uint32_t r = (val & 0xFF0000) >> 16;
    uint32_t g = (val & 0x00FF00) >> 8;
    uint32_t b = (val) & 0x0000FF;

    uint32_t y = (0.299f * r) + (0.587f * g) + (0.114f * b);
    uint32_t u = ((-0.169f * r) + (-0.331 * g) + (0.5f * b)) + 128;
    uint32_t v = ((0.5f * r) + (-0.419f * g) + (-0.081f * b)) + 128;
    return (y << 16) + (u << 8) + v;
}

static inline bool yuvDifference(glm::uvec3 lhs, glm::uvec3 rhs) {
    glm::ivec3 lhs_yuv = rgbToYuv(lhs);
    glm::ivec3 rhs_yuv = rgbToYuv(rhs);
    return (abs(lhs_yuv.x - rhs_yuv.x) > y_threshold ||
            abs(lhs_yuv.y - rhs_yuv.y) > u_threshold ||
            abs(lhs_yuv.z - rhs_yuv.z) > v_threshold);
}

template<typename T>
static inline T interpolate2Pixels(T c1, int32_t w1, T c2, int32_t w2, int32_t s) {
    if (c1 == c2) { return c1; }
    return {
        ((c1.r * w1) + (c2.r * w2)) >> s,
        ((c1.g * w1) + (c2.g * w2)) >> s,
        ((c1.b * w1) + (c2.b * w2)) >> s};
}

template<typename T>
static inline T interpolate3Pixels(T c1, int32_t w1, T c2, int32_t w2, T c3, int32_t w3, int32_t s) {
    return {
        ((c1.r * w1) + (c2.r * w2) + (c3.r * w3)) >> s,
        ((c1.g * w1) + (c2.g * w2) + (c3.g * w3)) >> s,
        ((c1.b * w1) + (c2.b * w2) + (c3.b * w3)) >> s};
}

/**
 * Create 8-bit 'mask' representing which pixels (sans w[4]) have a difference with w[4] (the center pixel)
 * 
 * @param w Array containing the 3x3 grid of pixels to be examined
 * 
 * @return 8-bit mask where a pixel with a difference is indicated by a 1, 0 otherwise.
 * Example: w[0], w[2], and w[5] ONLY are different: 00010101
 */
template<typename T>
static uint8_t compute_differences(std::array<T, 9> w) {
    uint8_t diffs = 0U;
    for (uint8_t offset = 0U; offset < 9; offset++) {
        if (offset == 4) { continue; }

        bool pixel_diff = yuvDifference(w[4], w[offset]);
        if (offset < 4) { diffs |= (pixel_diff << offset); } else { diffs |= (pixel_diff << (offset - 1)); }
    }
    return diffs;
}

template<typename T>
Image<T> scaleHq2x(const Image<T>& src) {
    auto result = Image<T>(src.width * 2, src.height * 2);

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            // Acquire original pixel grid values (row by row)
            std::array<T, 9> w;
            size_t counter = 0UL;
            for (int y_offset = -1; y_offset <= 1; y_offset++) {
                for (int x_offset = -1; x_offset <= 1; x_offset++) {
                    w[counter++] = src.safeAccess(x + x_offset, y + y_offset, NEAREST);
                }
            }

            // Compute conditions corresponding to each set of 2x2 interpolation rules in reduced problem set
            uint8_t diffs = compute_differences(w);
            const bool cond00 = (P(0xbf,0x37) || P(0xdb,0x13)) && WDIFF(w[1], w[5]);
            const bool cond01 = (P(0xdb,0x49) || P(0xef,0x6d)) && WDIFF(w[7], w[3]);
            const bool cond02 = (P(0x6f,0x2a) || P(0x5b,0x0a) || P(0xbf,0x3a) ||
                                P(0xdf,0x5a) || P(0x9f,0x8a) || P(0xcf,0x8a) ||
                                P(0xef,0x4e) || P(0x3f,0x0e) || P(0xfb,0x5a) ||
                                P(0xbb,0x8a) || P(0x7f,0x5a) || P(0xaf,0x8a) ||
                                P(0xeb,0x8a)) && WDIFF(w[3], w[1]);
            const bool cond03 = P(0xdb,0x49) || P(0xef,0x6d);
            const bool cond04 = P(0xbf,0x37) || P(0xdb,0x13);
            const bool cond05 = P(0x1b,0x03) || P(0x4f,0x43) || P(0x8b,0x83) ||
                            P(0x6b,0x43);
            const bool cond06 = P(0x4b,0x09) || P(0x8b,0x89) || P(0x1f,0x19) ||
                            P(0x3b,0x19);
            const bool cond07 = P(0x0b,0x08) || P(0xf9,0x68) || P(0xf3,0x62) ||
                            P(0x6d,0x6c) || P(0x67,0x66) || P(0x3d,0x3c) ||
                            P(0x37,0x36) || P(0xf9,0xf8) || P(0xdd,0xdc) ||
                            P(0xf3,0xf2) || P(0xd7,0xd6) || P(0xdd,0x1c) ||
                            P(0xd7,0x16) || P(0x0b,0x02);
            const bool cond08 = (P(0x0f,0x0b) || P(0x2b,0x0b) || P(0xfe,0x4a) ||
                                P(0xfe,0x1a)) && WDIFF(w[3], w[1]);
            const bool cond09 = P(0x2f,0x2f);
            const bool cond10 = P(0x0a,0x00);
            const bool cond11 = P(0x0b,0x09);
            const bool cond12 = P(0x7e,0x2a) || P(0xef,0xab);
            const bool cond13 = P(0xbf,0x8f) || P(0x7e,0x0e);
            const bool cond14 = P(0x4f,0x4b) || P(0x9f,0x1b) || P(0x2f,0x0b) ||
                            P(0xbe,0x0a) || P(0xee,0x0a) || P(0x7e,0x0a) ||
                            P(0xeb,0x4b) || P(0x3b,0x1b);
            const bool cond15 = P(0x0b,0x03);

            // Assign destination pixel values corresponding to the various conditions
            T dst00, dst01, dst10, dst11;

            if (cond00)
                dst00 = interpolate2Pixels(w[4], 5, w[3], 3, 3);
            else if (cond01)
                dst00 = interpolate2Pixels(w[4], 5, w[1], 3, 3);
            else if ((P(0x0b,0x0b) || P(0xfe,0x4a) || P(0xfe,0x1a)) && WDIFF(w[3], w[1]))
                dst00 = w[4];
            else if (cond02)
                dst00 = interpolate2Pixels(w[4], 5, w[0], 3, 3);
            else if (cond03)
                dst00 = interpolate2Pixels(w[4], 3, w[3], 1, 2);
            else if (cond04)
                dst00 = interpolate2Pixels(w[4], 3, w[1], 1, 2);
            else if (cond05)
                dst00 = interpolate2Pixels(w[4], 5, w[3], 3, 3);
            else if (cond06)
                dst00 = interpolate2Pixels(w[4], 5, w[1], 3, 3);
            else if (P(0x0f,0x0b) || P(0x5e,0x0a) || P(0x2b,0x0b) || P(0xbe,0x0a) ||
                    P(0x7a,0x0a) || P(0xee,0x0a))
                dst00 = interpolate2Pixels(w[1], 1, w[3], 1, 1);
            else if (cond07)
                dst00 = interpolate2Pixels(w[4], 5, w[0], 3, 3);
            else
                dst00 = interpolate3Pixels(w[4], 2, w[1], 1, w[3], 1, 2);

            if (cond00)
                dst01 = interpolate2Pixels(w[4], 7, w[3], 1, 3);
            else if (cond08)
                dst01 = w[4];
            else if (cond02)
                dst01 = interpolate2Pixels(w[4], 3, w[0], 1, 2);
            else if (cond09)
                dst01 = w[4];
            else if (cond10)
                dst01 = interpolate3Pixels(w[4], 5, w[1], 2, w[3], 1, 3);
            else if (P(0x0b,0x08))
                dst01 = interpolate3Pixels(w[4], 5, w[1], 2, w[0], 1, 3);
            else if (cond11)
                dst01 = interpolate2Pixels(w[4], 5, w[1], 3, 3);
            else if (cond04)
                dst01 = interpolate2Pixels(w[1], 3, w[4], 1, 2);
            else if (cond12)
                dst01 = interpolate3Pixels(w[1], 2, w[4], 1, w[3], 1, 2);
            else if (cond13)
                dst01 = interpolate2Pixels(w[1], 5, w[3], 3, 3);
            else if (cond05)
                dst01 = interpolate2Pixels(w[4], 7, w[3], 1, 3);
            else if (P(0xf3,0x62) || P(0x67,0x66) || P(0x37,0x36) || P(0xf3,0xf2) ||
                    P(0xd7,0xd6) || P(0xd7,0x16) || P(0x0b,0x02))
                dst01 = interpolate2Pixels(w[4], 3, w[0], 1, 2);
            else if (cond14)
                dst01 = interpolate2Pixels(w[1], 1, w[4], 1, 1);
            else
                dst01 = interpolate2Pixels(w[4], 3, w[1], 1, 2);

            if (cond01)
                dst10 = interpolate2Pixels(w[4], 7, w[1], 1, 3);
            else if (cond08)
                dst10 = w[4];
            else if (cond02)
                dst10 = interpolate2Pixels(w[4], 3, w[0], 1, 2);
            else if (cond09)
                dst10 = w[4];
            else if (cond10)
                dst10 = interpolate3Pixels(w[4], 5, w[3], 2, w[1], 1, 3);
            else if (P(0x0b,0x02))
                dst10 = interpolate3Pixels(w[4], 5, w[3], 2, w[0], 1, 3);
            else if (cond15)
                dst10 = interpolate2Pixels(w[4], 5, w[3], 3, 3);
            else if (cond03)
                dst10 = interpolate2Pixels(w[3], 3, w[4], 1, 2);
            else if (cond13)
                dst10 = interpolate3Pixels(w[3], 2, w[4], 1, w[1], 1, 2);
            else if (cond12)
                dst10 = interpolate2Pixels(w[3], 5, w[1], 3, 3);
            else if (cond06)
                dst10 = interpolate2Pixels(w[4], 7, w[1], 1, 3);
            else if (P(0x0b,0x08) || P(0xf9,0x68) || P(0x6d,0x6c) || P(0x3d,0x3c) ||
                    P(0xf9,0xf8) || P(0xdd,0xdc) || P(0xdd,0x1c))
                dst10 = interpolate2Pixels(w[4], 3, w[0], 1, 2);
            else if (cond14)
                dst10 = interpolate2Pixels(w[3], 1, w[4], 1, 1);
            else
                dst10 = interpolate2Pixels(w[4], 3, w[3], 1, 2);

            if ((P(0x7f,0x2b) || P(0xef,0xab) || P(0xbf,0x8f) || P(0x7f,0x0f)) &&
                WDIFF(w[3], w[1]))
                dst11 = w[4];
            else if (cond02)
                dst11 = interpolate2Pixels(w[4], 7, w[0], 1, 3);
            else if (cond15)
                dst11 = interpolate2Pixels(w[4], 7, w[3], 1, 3);
            else if (cond11)
                dst11 = interpolate2Pixels(w[4], 7, w[1], 1, 3);
            else if (P(0x0a,0x00) || P(0x7e,0x2a) || P(0xef,0xab) || P(0xbf,0x8f) ||
                    P(0x7e,0x0e))
                dst11 = interpolate3Pixels(w[4], 6, w[3], 1, w[1], 1, 3);
            else if (cond07)
                dst11 = interpolate2Pixels(w[4], 7, w[0], 1, 3);
            else
                dst11 = w[4];

            // Final assignments
            int dst_x = 2 * x;
            int dst_y = 2 * y;
            result.data[result.getImageOffset(dst_x, dst_y)]            = dst00;
            result.data[result.getImageOffset(dst_x + 1, dst_y)]        = dst01;
            result.data[result.getImageOffset(dst_x, dst_y + 1)]        = dst10;
            result.data[result.getImageOffset(dst_x + 1, dst_y + 1)]    = dst11;
        }
    }

    return result;
}

#endif

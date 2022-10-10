#ifndef COMMON_HPP
#define COMMON_HPP

#include <stdint.h>

template<typename T>
bool threeOrMoreIdentical(T a, T b, T c, T d) {
    return ((a == b && b == c) ||
            (a == b && b == d) ||
            (a == c && c == b) ||
            (a == c && c == d) ||
            (a == d && d == b) ||
            (a == d && d == c) ||
            (b == a && a == c) ||
            (b == a && a == d) ||
            (b == c && c == a) ||
            (b == c && c == d) ||
            (b == d && d == a) ||
            (b == d && d == c) ||
            (c == a && a == b) ||
            (c == a && a == d) ||
            (c == b && b == a) ||
            (c == b && b == d) ||
            (c == d && d == a) ||
            (c == d && d == b) ||
            (d == a && a == b) ||
            (d == a && a == c) ||
            (d == b && b == a) ||
            (d == b && b == c) ||
            (d == c && c == a) ||
            (d == c && c == b));
}

template<typename T>
inline T bilinearInterpolation(T top_left, T top_right, T bottom_left, T bottom_right,
                               float right_proportion, float bottom_proportion) {
    T top_interp = glm::mix(top_left, top_right, right_proportion);
    T bottom_interp = glm::mix(bottom_left, bottom_right, right_proportion);
    return glm::mix(top_interp, bottom_interp, bottom_proportion);
}

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

#endif

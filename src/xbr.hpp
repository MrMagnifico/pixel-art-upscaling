#ifndef XBR_HPP
#define XBR_HPP

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include "common.hpp"

constexpr uint8_t Y_COEFF = 0x30;
constexpr uint8_t U_COEFF = 0x07;
constexpr uint8_t V_COEFF = 0x06;

uint32_t dist(glm::uvec3 A, glm::uvec3 B) {
    glm::ivec3 A_yuv = rgbToYuv(A);
    glm::ivec3 B_yuv = rgbToYuv(B);
    glm::uvec3 diff = glm::abs(A_yuv - B_yuv);
    return (diff.x * Y_COEFF) + (diff.y * U_COEFF) + (diff.z * V_COEFF);
}

template<typename T>
Image<T> scaleXbr(const Image<T>& src) {
    auto result = Image<T>(src.width * 2, src.height * 2);

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            // Acquire original pixel grid values (row by row)
            T A1, B1, C1;
            A1 = src.safeAccess(x - 1, y - 2, NEAREST), B1 = src.safeAccess(x, y - 2, NEAREST), C1 = src.safeAccess(x + 1, y - 2, NEAREST);
            T A0, A, B, C, C4;
            A0 = src.safeAccess(x - 2, y - 1, NEAREST), A = src.safeAccess(x - 1, y - 1, NEAREST), B = src.safeAccess(x, y - 1, NEAREST),
            C = src.safeAccess(x + 1, y - 1, NEAREST), C4 = src.safeAccess(x + 2, y - 1, NEAREST);
            T D0, D, E, F, F4;
            D0 = src.safeAccess(x - 2, y, NEAREST), D = src.safeAccess(x - 1, y, NEAREST), E = src.safeAccess(x, y, NEAREST),
            F = src.safeAccess(x + 1, y, NEAREST), F4 = src.safeAccess(x + 2, y, NEAREST);
            T G0, G, H, I, I4;
            G0 = src.safeAccess(x - 2, y + 1, NEAREST), G = src.safeAccess(x - 1, y + 1, NEAREST), H = src.safeAccess(x, y + 1, NEAREST),
            I = src.safeAccess(x + 1, y + 1, NEAREST), I4 = src.safeAccess(x + 2, y + 1, NEAREST);
            T G5, H5, I5;
            G5 = src.safeAccess(x - 1, y + 2, NEAREST), H5 = src.safeAccess(x, y + 2, NEAREST), I5 = src.safeAccess(x + 1, y + 2, NEAREST);

            // Detect diagonal edges in the four possible directions
            uint32_t bot_right_perpendicular_dist   = dist(E, C) + dist(E, G) + dist(I, F4) + dist(I, H5) + 4 * dist(H, F);
            uint32_t bot_right_parallel_dist        = dist(H, D) + dist(H, I5) + dist(F, I4) + dist(F, B) + 4 * dist(E, I);
            bool edr_bot_right                      = bot_right_perpendicular_dist < bot_right_parallel_dist;
            uint32_t bot_left_perpendicular_dist    = dist(A, E) + dist(E, I) + dist(D0, G) + dist(G, H5) + 4 * dist(D, H);
            uint32_t bot_left_parallel_dist         = dist(B, D) + dist(F, H) + dist(D, G0) + dist(H, G5) + 4 * dist(E, G);
            bool edr_bot_left                       = bot_left_perpendicular_dist < bot_left_parallel_dist;
            uint32_t top_left_perpendicular_dist    = dist(G, E) + dist(E, C) + dist(D0, A) + dist(A, B1) + 4 * dist(D, B);
            uint32_t top_left_parallel_dist         = dist(H, D) + dist(D, A0) + dist(F, B) + dist(B, A1) + 4 * dist(E, A);
            bool edr_top_left                       = top_left_perpendicular_dist < top_left_parallel_dist;
            uint32_t top_right_perpendicular_dist   = dist(A, E) + dist(E, I) + dist(B1, C) + dist(C, F4) + 4 * dist(B, F);
            uint32_t top_right_parallel_dist        = dist(D, B) + dist(B, C1) + dist(H, F) + dist(F, C4) + 4 * dist(E, C);
            bool edr_top_right                      = top_right_perpendicular_dist < top_right_parallel_dist;

            // Initial values are same as pixel being expanded
            T zero, one, two, three;
            zero = one = two = three = src.safeAccess(x, y);

            // TODO: The whole colour rule section is a MAJOR source of error
            if (edr_bot_right) {
                T new_color = (dist(E, F) <= dist(E, H)) ? F : H;
                if (F == G && H == C) {
                    // TODO: Actually figure out what the fuck this is supposed to be
                    three   = glm::mix(three, new_color, 0.75f); // Mix the mix?
                    two     = glm::mix(two, new_color, 0.25f);
                    one     = glm::mix(one, new_color, 0.25f);
                } else if (F == G) {
                    three   = glm::mix(three, new_color, 0.75f);
                    two     = glm::mix(two, new_color, 0.25f);
                } else if (H == C) {
                    three   = glm::mix(three, new_color, 0.75f);
                    one     = glm::mix(one, new_color, 0.25f);
                } else { three = glm::mix(three, new_color, 0.5f); }
            }
            if (edr_bot_left) {
                T new_color = (dist(E, H) <= dist(E, D)) ? H : D;
                if (A == H && D == I) {
                    two     = glm::mix(two, new_color, 0.75f);
                    zero    = glm::mix(zero, new_color, 0.25f);
                    three   = glm::mix(three, new_color, 0.25f);
                } else if (A == H) {
                    two     = glm::mix(two, new_color, 0.75f);
                    zero    = glm::mix(zero, new_color, 0.25f);
                } else if (D == I) {
                    two     = glm::mix(two, new_color, 0.75f);
                    three   = glm::mix(three, new_color, 0.25f);
                } else { two = glm::mix(two, new_color, 0.5f); }
            }
            if (edr_top_left) {
                T new_color = (dist(E, D) <= dist(E, B)) ? D : B;
                if (D == C && B == G) {
                    zero    = glm::mix(zero, new_color, 0.75f);
                    one     = glm::mix(one, new_color, 0.25f);
                    two     = glm::mix(two, new_color, 0.25f);
                } else if (D == C) {
                    zero    = glm::mix(zero, new_color, 0.75f);
                    one     = glm::mix(one, new_color, 0.25f);
                } else if (B == G) {
                    zero    = glm::mix(zero, new_color, 0.75f);
                    two     = glm::mix(two, new_color, 0.25f);
                } else { zero = glm::mix(zero, new_color, 0.5f); }
            }
            if (edr_top_right) {
                T new_color = (dist(E, B) <= dist(E, F)) ? B : F;
                if (B == I && F == A) {
                    one     = glm::mix(one, new_color, 0.75f);
                    three   = glm::mix(three, new_color, 0.25f);
                    zero    = glm::mix(zero, new_color, 0.25f);
                } else if (B == I) {
                    one     = glm::mix(one, new_color, 0.75f);
                    three   = glm::mix(three, new_color, 0.25f);
                } else if (F == A) {
                    one     = glm::mix(one, new_color, 0.75f);
                    zero    = glm::mix(zero, new_color, 0.25f);
                } else { one = glm::mix(one, new_color, 0.5f); }
            }

            // Final assignments
            int dst_x = 2 * x;
            int dst_y = 2 * y;
            result.data[result.getImageOffset(dst_x, dst_y)]            = zero;
            result.data[result.getImageOffset(dst_x + 1, dst_y)]        = one;
            result.data[result.getImageOffset(dst_x, dst_y + 1)]        = two;
            result.data[result.getImageOffset(dst_x + 1, dst_y + 1)]    = three;
        }
    }

    return result;
}

#endif

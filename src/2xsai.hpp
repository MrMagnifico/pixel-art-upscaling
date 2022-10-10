#ifndef _2XSAI_HPP
#define _2XSAI_HPP

#include <framework/disable_all_warnings.h>
#include <framework/image.h>

#include "common.hpp"

/**
 * Compute if C and D exclusively match either A or B.
 * 
 * @param A First candidate for majority match
 * @param B Second candidate for majority match
 * @param C First value to check for majority match
 * @param D Second value to check for majority match
 * 
 * @return 0 if no majority match on A or B; 1 if majority match ONLY on A; -1 if majority match ONLY on B
 */
template<typename T>
inline int8_t majorityMatch(T A, T B, T C, T D) {
    int8_t x, y, r;
    x = y = r = 0;
    if (A == C) { x += 1; } else if (B == C) { y += 1; }
    if (A == D) { x += 1; } else if (B == D) { y += 1; }
    if (x <= 1) { r -=1 ; } 
    if (y <= 1) { r +=1 ; }
    return r;
}

template<typename T>
Image<T> scale2xSaI(const Image<T>& src) {
    auto result = Image<T>(src.width * 2, src.height * 2);

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            // Acquire original pixel grid values (row by row)
            T I, E, F, J;
            I = src.safeAccess(x - 1, y - 1), E = src.safeAccess(x, y - 1), F = src.safeAccess(x + 1, y - 1), J = src.safeAccess(x + 2, y - 1);
            T G, A, B, K;
            G = src.safeAccess(x - 1, y), A = src.safeAccess(x, y), B = src.safeAccess(x + 1, y), K = src.safeAccess(x + 2, y);
            T H, C, D, L;
            H = src.safeAccess(x - 1, y + 1), C = src.safeAccess(x, y + 1), D = src.safeAccess(x + 1, y + 1), L = src.safeAccess(x + 2, y + 1);
            T M, N, O, P;
            M = src.safeAccess(x - 1, y + 2), N = src.safeAccess(x, y + 2), O = src.safeAccess(x + 1, y + 2), P = src.safeAccess(x + 2, y + 2);

            T right_interp, bottom_interp, bottom_right_interp;

            // First filter layer: check for edges (i.e. same colour) along A-D and B-C edge
            // Second filter layer: acquire concrete values for interpolated pixels based on matching of neighbour pixel colours
            if (A == D && B != C) {
                if ((A == E && B == L) || (A == C && A == F && B != E && B == J)) { right_interp = A; }
                else { right_interp = glm::mix(A, B, 0.50f); }

                if ((A == G && C == O) || (A == B && A == H && G != C && C == M)) { bottom_interp = A; }
                else { bottom_interp = A; }

                bottom_right_interp = A;
            } else if (A != D && B == C) {
                if ((B == F && A == H) || (B == E && B == D && A != F && A == I)) { right_interp = B; }
                else { right_interp = glm::mix(A, B, 0.5f); }

                if ((C == H && A == F) || (C == G && C == D && A != H && A == I)) { bottom_interp = C; }
                else { bottom_interp = glm::mix(A, C, 0.5f); }

                bottom_right_interp = B;
            } else if (A == D && B == C) {
                if (A == B) { right_interp = bottom_interp = bottom_right_interp = A; }
                else {
                    right_interp = glm::mix(A, B, 0.5f);

                    bottom_interp = glm::mix(A, C, 0.5f);

                    int8_t majority_accumulator = 0;
                    majority_accumulator += majorityMatch(B, A, G, E);
                    majority_accumulator += majorityMatch(B, A, K, F);
                    majority_accumulator += majorityMatch(B, A, H, N);
                    majority_accumulator += majorityMatch(B, A, L, O);
                    if (majority_accumulator > 0) { bottom_right_interp = A; }
                    else if (majority_accumulator < 0) { bottom_right_interp = B; }
                    else { bottom_right_interp = bilinearInterpolation(A, B, C, D, 0.5f, 0.5f); }
                }
            } else {
                bottom_right_interp = bilinearInterpolation(A, B, C, D, 0.5f, 0.5f);

                if (A == C && A == F && B != E && B == J) { right_interp = A; }
                else if (B == E && B == D && A != F && A == I) { right_interp = B; }
                else { right_interp = glm::mix(A, B, 0.5f); }

                if (A == B && A == H && G != C && C == M) { bottom_interp = A; }
                else if (C == G && C == D && A != H && A == I) { bottom_interp = C; }
                else { bottom_interp = glm::mix(A, C, 0.5f); }
            }

            int dst_x = 2 * x;
            int dst_y = 2 * y;
            result.data[result.getImageOffset(dst_x, dst_y)]            = A;
            result.data[result.getImageOffset(dst_x + 1, dst_y)]        = right_interp;
            result.data[result.getImageOffset(dst_x, dst_y + 1)]        = bottom_interp;
            result.data[result.getImageOffset(dst_x + 1, dst_y + 1)]    = bottom_right_interp;
        }
    }
    return result;
}

#endif

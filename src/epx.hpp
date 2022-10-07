#ifndef EPX_HPP
#define EPX_HPP

#include <framework/disable_all_warnings.h>
#include <framework/image.h>

#include "common.hpp"

template<typename T>
Image<T> scaleEpx(const Image<T>& src) {
    auto result = Image<T>(src.width * 2, src.height * 2);

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            // Acquire neighbour pixel values
            T A = src.safeAccess(x, y - 1);
            T B = src.safeAccess(x + 1, y);
            T C = src.safeAccess(x - 1, y);
            T D = src.safeAccess(x, y + 1);

            // Initial expanded pixel value assignments
            T original_pixel = src.safeAccess(x, y);
            T one, two, three, four;
            one = two = three = four = original_pixel;

            // Interpolation conditions
            if (C == A) { one = A; }
            if (A == B) { two = B; }
            if (D == C) { three = C; }
            if (B == D) { four = D; }
            if (threeOrMoreIdentical(A, B, C, D)) { one = two = three = four = original_pixel; }

            int dst_x = 2 * x;
            int dst_y = 2 * y;
            result.data[result.getImageOffset(dst_x, dst_y)]            = one;
            result.data[result.getImageOffset(dst_x + 1, dst_y)]        = two;
            result.data[result.getImageOffset(dst_x, dst_y + 1)]        = three;
            result.data[result.getImageOffset(dst_x + 1, dst_y + 1)]    = four;
        }
    }
    return result;
}

template<typename T>
Image<T> scaleAdvMame(const Image<T>& src) {
    auto result = Image<T>(src.width * 2, src.height * 2);

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            // Acquire neighbour pixel values
            T A = src.safeAccess(x, y - 1);
            T B = src.safeAccess(x + 1, y);
            T C = src.safeAccess(x - 1, y);
            T D = src.safeAccess(x, y + 1);

            // Initial expanded pixel value assignments
            T original_pixel = src.safeAccess(x, y);
            T one, two, three, four;
            one = two = three = four = original_pixel;

            // Interpolation conditions
            if (C == A && C != D && A != B) { one = A; }
            if (A == B && A != C && B != D) { two = B; }
            if (D == C && D != B && C != A) { three = C; }
            if (B == D && B != A && D != C) { four = D; }

            int dst_x = 2 * x;
            int dst_y = 2 * y;
            result.data[result.getImageOffset(dst_x, dst_y)]            = one;
            result.data[result.getImageOffset(dst_x + 1, dst_y)]        = two;
            result.data[result.getImageOffset(dst_x, dst_y + 1)]        = three;
            result.data[result.getImageOffset(dst_x + 1, dst_y + 1)]    = four;
        }
    }
    return result;
}

#endif

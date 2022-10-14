#ifndef NEDI_HPP
#define NEDI_HPP

#include <array>
#include <utility>

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <Eigen/Core>
#include <Eigen/Dense>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include "common.hpp"


typedef std::pair<size_t, size_t> ImageCoords;

constexpr size_t WINDOW_PXL_LENGTH = 4U; // Must be even


namespace Eigen {
    template<> struct NumTraits<glm::uvec3> : NumTraits<uint32_t> {
        typedef glm::uvec3 Real;
        typedef glm::vec3 NonInteger;
        typedef glm::uvec3 Literal;
        typedef glm::uvec3 Nested;
    
        enum {
            IsComplex = 0,
            IsInteger = 1,
            IsSigned = 0,
            RequireInitialization = 1,
            ReadCost = 1,
            AddCost = 2,
            MulCost = 2
        };
    };

};

namespace Eigen {
    template<> struct NumTraits<glm::vec3> : NumTraits<float> {
        typedef glm::vec3 Real;
        typedef glm::vec3 NonInteger;
        typedef glm::vec3 Literal;
        typedef glm::vec3 Nested;
    
        enum {
            IsComplex = 0,
            IsInteger = 0,
            IsSigned = 1,
            RequireInitialization = 1,
            ReadCost = 1,
            AddCost = 3,
            MulCost = 3
        };
    };
};


template<typename T>
Image<T> scaleNedi(const Image<T>& src) {
    auto result = Image<T>(src.width * 2, src.height * 2);

    // Solve for 25% of pixels (top-left corner of 2x2 block) - needed for subsequent interpolation of 'b' pixels
    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            result.data[result.getImageOffset(2*x, 2*y)] = src.safeAccess(x, y);
        }
    }

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            // Define top left corner of the sampling box
            int top_left_x = x - ((WINDOW_PXL_LENGTH / 2) - 1);
            int top_left_y = y - ((WINDOW_PXL_LENGTH / 2) - 1);
            ImageCoords top_left_pixel_xy(top_left_x >= 0 ? top_left_x : 0,
                                          top_left_y >= 0 ? top_left_y : 0);

            // Construct column vector representing window and matrices representing diagonal and axial neighbours of each pixel in the window
            Eigen::Matrix<T, WINDOW_PXL_LENGTH * WINDOW_PXL_LENGTH, 1> col_vec_y;
            Eigen::Matrix<T, WINDOW_PXL_LENGTH * WINDOW_PXL_LENGTH, 4> diagonal_neighbours;
            Eigen::Matrix<T, WINDOW_PXL_LENGTH * WINDOW_PXL_LENGTH, 4> axial_neighbours;
            size_t window_pixel_counter = 0U;
            for (int offset_y = 0; offset_y < WINDOW_PXL_LENGTH; offset_y++) {
                for (int offset_x = 0; offset_x < WINDOW_PXL_LENGTH; offset_x++) {
                    int window_pixel_x = top_left_pixel_xy.first + offset_x;
                    int window_pixel_y = top_left_pixel_xy.second + offset_y;

                    col_vec_y(window_pixel_counter) = src.safeAccess(window_pixel_x, window_pixel_y, NEAREST);
                    std::array<T, 4> diagonal_neighbour_row = {
                        src.safeAccess(window_pixel_x - 1, window_pixel_y - 1, NEAREST),
                        src.safeAccess(window_pixel_x + 1, window_pixel_y - 1, NEAREST),
                        src.safeAccess(window_pixel_x - 1, window_pixel_y + 1, NEAREST),
                        src.safeAccess(window_pixel_x + 1, window_pixel_y + 1, NEAREST)};
                    for (uint8_t col = 0; col < 4; col++) { diagonal_neighbours(window_pixel_counter, col) = diagonal_neighbour_row[col]; }
                    std::array<T, 4> axial_neighbour_row = {
                        src.safeAccess(window_pixel_x, window_pixel_y - 1, NEAREST),
                        src.safeAccess(window_pixel_x - 1, window_pixel_y, NEAREST),
                        src.safeAccess(window_pixel_x + 1, window_pixel_y, NEAREST),
                        src.safeAccess(window_pixel_x, window_pixel_y + 1, NEAREST)};
                    for (uint8_t col = 0; col < 4; col++) { axial_neighbours(window_pixel_counter, col) = axial_neighbour_row[col]; }

                    window_pixel_counter++;
                }
            }

            // Compute diagonal and axial interpolation weights
            auto diagonal_neighbours_transpose  = diagonal_neighbours.transpose();
            auto axial_neighbours_transpose     = axial_neighbours.transpose();
            auto diagonal_lhs                   = (diagonal_neighbours_transpose * diagonal_neighbours).inverse();
            auto axial_lhs                      = (axial_neighbours_transpose * axial_neighbours).inverse();
            Eigen::Matrix<T, 4, 1> diagonal_interp_weights  = diagonal_lhs * (diagonal_neighbours_transpose * col_vec_y);
            Eigen::Matrix<T, 4, 1> axial_interp_weights     = axial_lhs    * (axial_neighbours_transpose * col_vec_y); 
            
            int dst_x;
            int dst_y;

            // 'b' pixel so diagonal neighbours
            dst_x = (2 * x) + 1;
            dst_y = (2 * y) + 1;
            Eigen::Matrix<T, 4, 1> interp_bot_right_pixels { result.safeAccess(dst_x - 1, dst_y - 1),
                                                             result.safeAccess(dst_x + 1, dst_y - 1),
                                                             result.safeAccess(dst_x - 1, dst_y + 1),
                                                             result.safeAccess(dst_x + 1, dst_y + 1)};
            T interp_bot_right = diagonal_interp_weights.dot(interp_bot_right_pixels);
            result.data[result.getImageOffset(dst_x, dst_y)] = interp_bot_right;
            
            // 'a' pixel so axial neighbours
            dst_x = (2 * x) + 1;
            dst_y = (2 * y);
            Eigen::Matrix<T, 4, 1> interp_top_right_pixels { result.safeAccess(dst_x, dst_y - 1),
                                                             result.safeAccess(dst_x - 1, dst_y),
                                                             result.safeAccess(dst_x + 1, dst_y),
                                                             result.safeAccess(dst_x, dst_y + 1)};
            T interp_top_right = axial_interp_weights.dot(interp_top_right_pixels);
            result.data[result.getImageOffset(dst_x, dst_y)] = interp_top_right;
            
            // 'a' pixel so axial neighbours
            dst_x = (2 * x);
            dst_y = (2 * y) + 1;
            Eigen::Matrix<T, 4, 1> interp_bot_left_pixels { result.safeAccess(dst_x, dst_y - 1),
                                                            result.safeAccess(dst_x - 1, dst_y),
                                                            result.safeAccess(dst_x + 1, dst_y),
                                                            result.safeAccess(dst_x, dst_y + 1)};
            T interp_bot_left = axial_interp_weights.dot(interp_bot_left_pixels);
            result.data[result.getImageOffset(dst_x, dst_y)] = interp_bot_left;
        }
    }

    return result;
}


#endif

#ifndef NEDI_HPP
#define NEDI_HPP

#include <array>
#include <utility>

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/LU>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include "common.hpp"


constexpr glm::vec3 CONDITION_THRESHOLD(2.0f);
constexpr uint32_t WINDOW_SIZE_MAX = 8U;


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
void setAllRowsToValue(Eigen::Matrix<T, 4, 1>& col_vec, T value) {
    for (uint8_t i = 0; i < 4; i++) { col_vec[i] = value; }
}

/**
 * Compute the condition (as defined by the Adaptive NEDI paper) of the R matrices (as defined by the NEDI paper)
 * 
 * @param window_size Size of the local window used for covariance sampling
 * @param diagonal_neighbours Matrix containing the diagonal neighbours of the pixels in the sampling window
 * @param axial_neighbours Matrix containing the axial neighbours of the pixels in the sampling window
 * 
 * @return True if the conditions of both matrices are below CONDITION_THRESHOLD and are not NaNs, false otherwise
*/
template<typename T>
bool conditionBelowThreshold(uint32_t window_size, const Eigen::Matrix<T, Eigen::Dynamic, 4>& diagonal_neighbours,
                             const Eigen::Matrix<T, Eigen::Dynamic, 4>& axial_neighbours) {
    float scalar_component = 1.0f / (float(window_size) * float(window_size));
    Eigen::Matrix<T, 4, 4> R_diagonal   = (diagonal_neighbours.transpose() * diagonal_neighbours);
    Eigen::Matrix<T, 4, 4> R_axial      = (axial_neighbours.transpose() * axial_neighbours);

    T condition_diagonal    = (R_diagonal.norm() * R_diagonal.inverse().norm()) * scalar_component;
    T condition_axial       = (R_axial.norm() * R_axial.inverse().norm()) * scalar_component;
    return glm::all(glm::lessThan(condition_diagonal, CONDITION_THRESHOLD)) &&
           glm::all(glm::lessThan(condition_axial, CONDITION_THRESHOLD)) &&
           !glm::any(glm::isnan(condition_diagonal)) && !glm::any(glm::isnan(condition_axial));
}

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
            uint32_t window_pxl_length = 0U;

            // Define top left corner of the sampling box
            int top_left_x = x - ((window_pxl_length / 2) - 1);
            int top_left_y = y - ((window_pxl_length / 2) - 1);

            // Construct column vector representing window and matrices representing diagonal and axial neighbours of each pixel in the window
            Eigen::Matrix<T, Eigen::Dynamic, 1> col_vec_y;
            Eigen::Matrix<T, Eigen::Dynamic, 4> diagonal_neighbours;
            Eigen::Matrix<T, Eigen::Dynamic, 4> axial_neighbours;
            do {
                window_pxl_length   += 2U;
                col_vec_y           = Eigen::Matrix<T, Eigen::Dynamic, 1>(window_pxl_length * window_pxl_length, 1);
                diagonal_neighbours = Eigen::Matrix<T, Eigen::Dynamic, 4>(window_pxl_length * window_pxl_length, 4);
                axial_neighbours    = Eigen::Matrix<T, Eigen::Dynamic, 4>(window_pxl_length * window_pxl_length, 4);
                size_t window_pixel_counter = 0U;

                for (int offset_y = 0; offset_y < window_pxl_length; offset_y++) {
                    for (int offset_x = 0; offset_x < window_pxl_length; offset_x++) {
                        int window_pixel_x = top_left_x + offset_x;
                        int window_pixel_y = top_left_y + offset_y;

                        col_vec_y(window_pixel_counter) = src.safeAccess(window_pixel_x, window_pixel_y, NEAREST);
                        std::array<T, 4> diagonal_neighbour_row = {
                            src.safeAccess(window_pixel_x - 1, window_pixel_y - 1, ZERO),
                            src.safeAccess(window_pixel_x + 1, window_pixel_y - 1, ZERO),
                            src.safeAccess(window_pixel_x - 1, window_pixel_y + 1, ZERO),
                            src.safeAccess(window_pixel_x + 1, window_pixel_y + 1, ZERO)};
                        for (uint8_t col = 0; col < 4; col++) { diagonal_neighbours(window_pixel_counter, col) = diagonal_neighbour_row[col]; }
                        std::array<T, 4> axial_neighbour_row = {
                            src.safeAccess(window_pixel_x, window_pixel_y - 1, ZERO),
                            src.safeAccess(window_pixel_x - 1, window_pixel_y, ZERO),
                            src.safeAccess(window_pixel_x + 1, window_pixel_y, ZERO),
                            src.safeAccess(window_pixel_x, window_pixel_y + 1, ZERO)};
                        for (uint8_t col = 0; col < 4; col++) { axial_neighbours(window_pixel_counter, col) = axial_neighbour_row[col]; }

                        window_pixel_counter++;
                    }
                }
            } while (!conditionBelowThreshold(window_pxl_length, diagonal_neighbours, axial_neighbours) && window_pxl_length < WINDOW_SIZE_MAX);

            // Compute diagonal and axial interpolation weights left sub-term
            auto diagonal_neighbours_transpose  = diagonal_neighbours.transpose();
            auto axial_neighbours_transpose     = axial_neighbours.transpose();
            auto diagonal_lhs                   = (diagonal_neighbours_transpose * diagonal_neighbours).inverse();
            auto axial_lhs                      = (axial_neighbours_transpose * axial_neighbours).inverse();
            Eigen::Matrix<T, 4, 1> diagonal_interp_weights  = diagonal_lhs * (diagonal_neighbours_transpose * col_vec_y);
            Eigen::Matrix<T, 4, 1> axial_interp_weights     = axial_lhs    * (axial_neighbours_transpose * col_vec_y);

            // If either of the weight vectors has NaNs, replace with equal weights
            for (uint8_t i = 0; i < 4; i++) {
                if (glm::any(glm::isnan(diagonal_interp_weights(i)))) { setAllRowsToValue(diagonal_interp_weights, glm::vec3(0.25f)); }
                if (glm::any(glm::isnan(axial_interp_weights(i)))) { setAllRowsToValue(axial_interp_weights, glm::vec3(0.25f)); }
            }
            
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

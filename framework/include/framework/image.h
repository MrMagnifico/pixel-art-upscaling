#pragma once
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>

#include <filesystem>
#include <vector>
#include <cassert>
#include <exception>
#include <iostream>
#include <string>
#include <random>
#include <functional>

DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
DISABLE_WARNINGS_POP()

enum OutOfBoundsStrategy { ZERO, NEAREST };

template <typename T>
class Image {
public:
    Image(const std::filesystem::path& filePath);
    Image(const int new_width, const int new_height);
    Image(const Image&) = default;
    Image() : Image(1, 1) {};

    void writeToFile(const std::filesystem::path& filePath);
    size_t getImageOffset(int x, int y) const;
    T safeAccess(int x, int y, OutOfBoundsStrategy out_of_bounds_strategy = NEAREST) const;

public:
    int width, height;
    std::vector<T> data;
};

template<typename T> 
inline T stbToType(const stbi_uc* src) { throw std::runtime_error("Not implemented."); };
template <>
float stbToType<float>(const stbi_uc* src);
template <>
glm::vec3 stbToType<glm::vec3>(const stbi_uc* src);
template <>
glm::uvec3 stbToType<glm::uvec3>(const stbi_uc* src);

template<typename T>
inline T stbfToType(const float* src) { throw std::runtime_error("Not implemented."); };
template <>
float stbfToType<float>(const float* src);
template <>
glm::vec3 stbfToType<glm::vec3>(const float* src);
template <>
glm::uvec3 stbfToType<glm::uvec3>(const float* src);

template <typename T>
inline void typeToRgbUint8(stbi_uc* dst, const T& value) { throw std::runtime_error("Not implemented."); };
template <>
void typeToRgbUint8(stbi_uc* dst, const float& value);
template <>
void typeToRgbUint8(stbi_uc* dst, const glm::vec3& value);
template <>
void typeToRgbUint8(stbi_uc* dst, const glm::uvec3& value);

template <typename T>
inline T sampleNoise(std::function<float(void)>& pdf) { throw std::runtime_error("Not implemented."); };
template <>
inline float sampleNoise(std::function<float(void)>& pdf) { return pdf(); }
template <>
inline glm::vec3 sampleNoise(std::function<float(void)>& pdf) { return glm::vec3(pdf(), pdf(), pdf()); }

template <typename T>
Image<T>::Image(const std::filesystem::path& filePath)
{
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Image file " << filePath << " does not exists!" << std::endl;
        throw std::exception();
    }

    const auto filePathStr = filePath.string(); // Create l-value so c_str() is safe.
    int channels;

    if (stbi_is_hdr(filePathStr.c_str())) {
        stbi_hdr_to_ldr_gamma(1.0f);
        stbi_hdr_to_ldr_scale(1.0f);
        float* stb_data_float = stbi_loadf(filePathStr.c_str(), &width, &height, &channels, 0);

        data.resize(width * height);
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = stbfToType<T>(stb_data_float + i * channels);
        }

        stbi_image_free(stb_data_float);
    }
    else {
        stbi_uc* stb_data = stbi_load(filePathStr.c_str(), &width, &height, &channels, 0);
        if (!stb_data) {
            std::cerr << "Failed to read image " << filePath << " using stb_image.h" << std::endl;
            throw std::exception();
        }

        data.resize(width * height);
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = stbToType<T>(stb_data + i * channels);
        }

        stbi_image_free(stb_data);
    }


}

template <typename T>
Image<T>::Image(const int new_width, const int new_height)
{
    width = new_width;
    height = new_height;
    data.resize(width * height); // Should be full of zeros.
}

template <typename T>
inline void Image<T>::writeToFile(const std::filesystem::path& filePath) {

    // RGB => 3
    const auto channels = 3;

    // Converts floats to uint8 array. 
    // Assumes normalized format, so it is multiplied by 255 (on top of the scaling_factor).
    // If input is single channel, it triples it to get RGB.
    std::vector<stbi_uc> std_data;
    std_data.resize(width * height * channels);
    for (size_t i = 0; i < data.size(); i++) {
        typeToRgbUint8<T>(&std_data[i * channels], data[i]);
    }

    // Create a folder.
    if (!std::filesystem::is_directory(filePath.parent_path())) {
        std::filesystem::create_directories(filePath.parent_path());
    }

    // Decide JPG (default) or PNG based on extension.
    const auto filePathStr = filePath.string(); // Create l-value so c_str() is safe.
    if (filePath.extension() == ".png") {
        stbi_write_png(filePathStr.c_str(), width, height, channels, std_data.data(), width * channels);
    } else {
        stbi_write_jpg(filePathStr.c_str(), width, height, channels, std_data.data(), 95);
    }
};

template <typename T>
size_t Image<T>::getImageOffset(int x, int y) const { return x + (y * width); };

template <typename T>
T Image<T>::safeAccess(int x, int y, OutOfBoundsStrategy out_of_bounds_strategy) const {
    // Check if given coords are out of bounds
    bool out_of_bounds = (x < 0 || x >= width) || (y < 0 || y >= height);
    if (out_of_bounds) {
        switch(out_of_bounds_strategy) {
            case ZERO:
                return {};
            case NEAREST:
                size_t valid_offset = getImageOffset(std::clamp(x, 0, width - 1), std::clamp(y, 0, height - 1));
                return data[valid_offset];
        }
    }
    return data[getImageOffset(x, y)];
};

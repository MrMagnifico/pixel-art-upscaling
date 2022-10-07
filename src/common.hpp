#ifndef COMMON_HPP
#define COMMON_HPP

#include <stdint.h>

struct Pixel8bpc {
    uint8_t r, g, b;

    // Pixel8bpc(uint8_t value) : r(value), g(value), b(value) {};

    friend bool operator==(const Pixel8bpc& lhs, const Pixel8bpc& rhs) {
        return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b);
    }

    friend bool operator!=(const Pixel8bpc& lhs, const Pixel8bpc& rhs) {
        return !(lhs == rhs);
    }

    friend Pixel8bpc operator*(Pixel8bpc lhs, const Pixel8bpc& rhs) {
        return {lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b};
    }

    template<typename T>
    friend Pixel8bpc operator*(Pixel8bpc lhs, const T& rhs) {
        return {lhs.r * rhs, lhs.g * rhs, lhs.b * rhs};
    }

    Pixel8bpc& operator+=(const Pixel8bpc& rhs) {
        this->r += rhs.r;
        this->g += rhs.g;
        this->b += rhs.b;
        return *this;
    }
};

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

#endif

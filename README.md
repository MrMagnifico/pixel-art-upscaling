# CS4365 Final Project
This repository contains a portion of the implementation for a submission for the final project of the CS4365 - Applied Image Processing course. 

## Requirements
- CMake 3.11 or higher
- C++20 compliant compiler

## Group Member(s)
- William Narchi (5046122)

## Directory Structure
- `framework` contains a slightly modified version of the framework used by the Computer Graphics and Visualisation group at TU Delft for the assignments for CS4365 in addition to the following external libraries
    - `catch2`
    - `Eigen`
    - `fmt`
    - `glm`
    - `stb`
- `src` contains concrete implementations
    - `2xsai.hpp` contains an implementation of the '2x Scale and Interpolate Engine' by Derek Liauw Kie Fa
    - `common.hpp` contains functionality used across several of the implemented algorithms
    - `eagle.hpp` contains an implementation of the 'Eric's Pixel Expansion (EPX)' and 'AdvMAME2×' upscaling algorithms
    - `epx.hpp` contains an implementation of upscaling algorithm
    - `hq2x.hpp` contains an implementation of the hq2x upscaling algorithm by Maxim Stepin
    - `nedi.hpp` contains an implementation of the 'Adaptive New Edge-Directed Interpolation' by Fan-Yin Tzeng, which is based on the 'New Edge-Directed Interpolation' algorithm by Xin Li and Michael T. Orchard
    - `xbr.hpp` contains an implementation of the 2x version of the xBR algorithm by Hyllian

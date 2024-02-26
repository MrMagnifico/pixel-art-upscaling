# Pixel-art Upscaling
This repository contains C++ and Python implementations of various algorithms for upscaling pixel-art as part of the final project for CS4365 Applied Image Processing. For further details, kindly refer to the provided report.

## Group Member(s)
- William Narchi (5046122)

## Requirements
- Python 3.10 or higher (the implementation has been tested on Python 3.10.6 only, though is likely to work on older versions)
- CMake 3.11 or higher
- C++20 compliant compiler
- OpenMP 2.0 or higher
- Cairo graphics library

## Build Instructions
The C++ portion of the codebase uses CMake, so simply use your favourite CLI/IDE/code editor and compile the `fin-proj` target. All dependencies are included in this repository and compiled as needed.

For the Python portion of the codebase, simply install the packages specified in `requirements.txt` and run `main.py` from the root of this repository. You must ensure that the [Cairo graphics library](https://cairographics.org) is installed as well.

## Directory Structure
- `framework` contains a slightly modified version of the framework used by the Computer Graphics and Visualisation group at TU Delft for the assignments for CS4365 in addition to the following external libraries
    - `catch2`
    - `Eigen`
    - `fmt`
    - `glm`
    - `stb`
- `src` contains concrete implementations
  - C++
    - `2xsai.hpp` contains an implementation of the '2x Scale and Interpolate Engine' by Derek Liauw Kie Fa
    - `common.hpp` contains functionality used across several of the implemented algorithms
    - `eagle.hpp` contains an implementation of the Eagle upscaling algorithm
    - `epx.hpp` contains an implementation of the 'Eric's Pixel Expansion (EPX)' upscaling algorithm by Eric Johnston and the 'AdvMAME2x' algorithm
    - `hq2x.hpp` contains an implementation of the hq2x upscaling algorithm by Maxim Stepin
    - `nedi.hpp` contains an implementation of the 'Adaptive New Edge-Directed Interpolation' algorithm by Fan-Yin Tzeng, which is based on the 'New Edge-Directed Interpolation' algorithm by Xin Li and Michael T. Orchard
    - `xbr.hpp` contains an implementation of the 2x version of the xBR algorithm by Hylian
  - Python - implementation of the [Kopf-Lichinski pixel-art upscaling algorithm](http://johanneskopf.de/publications/pixelart/)
    - `geometry.py` contains functionality for creating and manipulating B-spline curves
    - `heuristics.py` allows for the computation of the diagonal edge cost heuristics specified in section 3.2 of the paper
    - `kopf_lichinksi.py` contains the bulk of the implementation, primarily localised within the `Vectorizer` class
    - `main.py` is the main entry point to the application and contains the code used to generate the data in the report
    - `pixel_io.py` contains functionality for loading images for upscaling (24-bit PNGs *without an alpha channel*) and outputing both SVG files and rasterized PNG files
    - `utils.py` contains various utilities used throughout different parts of the implementation

## Credits
The Python implementation is heavily based on
- [This Python implementation of the Kopf-Lischinski approach by Campbell Barton](https://github.com/jerith/depixel)
- [This Python 3 adaptation of Campbell Barton's implementation by Anirudh Vemula and Vamsidhar Yeddu](https://github.com/vvanirudh/Pixel-Art)

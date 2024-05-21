# ogl2

A bunch of tests around OpenGL and `grid_structure` - a grid-wise memory layout
for image data which intends to **localize data for processing of neighboring pixels with better performance.**

Test show 57% runtime compared to standard memory access patterns
for *large* images. (For small images the access overhead outweights
the cache hit improvement.)

## Build

    cmake -S . -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=~/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

## `grid_structure` and `testgs`

`grid_structure` is a grid where an image is subdivided into areas of (identical)
width `gw` and height `gh` which is a power of 2 (i.e. 2, 4, 8, 16, ...).

    /- gw -\
    ........########........########........ \
    ........########........########........  gh
    ........########........########........ /
    ########........########........########
    ########...P....########........########
    ########........########........########
    ........########........########........
    ........########........########........
    ........########........########........

Within each area the memory is laid out as usual with first the elements of the
top row from left to right, then the elements of the second row from left to
right and so on.

### Speed check

In a benchmark against the usual layout of image data the advantage of
cache-locality shows only under the following conditions
 - sufficiently large images ( >= 4096 * 4096 * float)
 - sufficiently many memory accesses to an optimum (enough to justify increased
 complexity of memory access pattern, not to much so memory is the bottleneck);
 in the tests 5x5 and 9x9 matrixes yielded the nearly identical runtimes.
 - area size of 16x16 or 32x32 seem to yield best results (i.e. `grid_structure<4>`
    or `grid_structure<5>`.) on a AMD Ryzen 5 3600 6-Core Processor (single
    threaded execution).

 Best run in `testgs` with 83% runtime for 4096x4096 image with 16x16 areas:

    101355/101355 passed. (0.00% failed)
    Test with 4096 x 4096 grid (256 x 256 areas of 16x16).
    Data amount of image: 64.000MB
    Data amount of area : 1.000KB
    Using 9x9 matrix with 81 + 2 memory accesses.
    Test with grid access    : 39.514s
    Test with linear access  : 47.486s
    grid : linear = 0.83:1

Best run in `testgs` with 57% runtime for 16384 x 16384 image (1 GB) with 16x16
areas:

    101355/101355 passed. (0.00% failed)
    Test with 16384 x 16384 grid (1024 x 1024 areas of 16x16 = 256 bytes per area).
    Data amount of image: 1024.000MB
    Data amount of area : 1.000KB
    Using 9x9 matrix with 81 + 2 memory accesses per pixel operation.
    Test with grid access    : 62.840s
    Test with linear access  : 110.498s
    grid : linear = 0.57:1

Increasing the image another time (2 GB) showed nearly the same difference in
performance boost:

    101355/101355 passed. (0.00% failed)
    Test with 32768 x 16384 grid (2048 x 1024 areas of 16x16 = 256 bytes per area).
    Data amount of image: 2048.000MB
    Data amount of area : 1.000KB
    Using 5x5 matrix with 25 + 2 memory accesses per pixel operation.
    Test with grid access    : 78.694s
    Test with linear access  : 134.678s
    grid : linear = 0.58:1

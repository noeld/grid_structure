#include <fmt/core.h>
#include <fmt/ranges.h>
#include "grid_structure.hpp"

int main(int argc, char const *argv[])
{

    unsigned coords[][2] = {{0, 0}, {1, 0}, {0, 1}, {7, 7,}, {8, 0}, {0, 8}, {1, 8}, {8, 8},
        {9, 0}, {0, 9}, {9, 9}};

    constexpr grid_structure<> gs(4, 4);
    for (auto & e : coords) {
        auto off1 = gs.coord_to_offset(e[0], e[1]);
        auto [x, y] = gs.offset_to_coord(off1);
        fmt::println("Testcase {::}: off: {} coord ({}, {})", e, off1, x, y);
    }

    return 0;
}
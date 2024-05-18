#include <fmt/core.h>
#include <fmt/ranges.h>
#include <functional>
#include <random>
#include "grid_structure.hpp"
#include <ranges>

int main(int argc, char const *argv[])
{

    unsigned coords[][2] = {{0, 0}, {1, 0}, {0, 1}, {7, 7,}, {8, 0}, {0, 8}, {1, 8}, {8, 8},
        {9, 0}, {0, 9}, {9, 9}};

    constexpr grid_structure<> gs(7, 3);

    auto test = [&](unsigned x0, unsigned y0, bool print_success = false) -> bool {
        auto off = gs.coord_to_offset(x0, y0);
        auto [x1, y1] = gs.offset_to_coord(off);
        bool result = std::tie(x0, y0) == std::tie(x1, y1);
        if (print_success || !result)
            fmt::println("{5:<10} Testcase ({0}, {1}): off: {2} coord ({3}, {4})", x0, y0, off, x1, y1, result ? "OK" : "Error");
        return result;
    };

    auto passed = 0, tested = 0;

    for (auto & e : coords) {
        ++tested;
        auto [x0, y0] = e;
        passed += test(x0, y0) ? 1 : 0;
    }


    auto mk_randomizer = [](unsigned max){
        std::mt19937_64 gen(std::random_device{}());
        std::uniform_int_distribution<unsigned> dist(0, max -1);
        auto rnd = std::bind(dist, gen);
        return [rnd]() mutable { return rnd(); };
    };

    auto rnd_x = mk_randomizer(gs.width());
    auto rnd_y = mk_randomizer(gs.height());
    constexpr size_t NUM_RANDOM_TESTS = 100000;
    for(auto i : std::views::iota(1) | std::views::take(NUM_RANDOM_TESTS)) {
        ++tested;
        passed += test(rnd_x(), rnd_y()) ? 1 : 0;
    }

    for(auto x = 0; x < gs.width(); ++x) {
        for(auto y = 0; y < gs.height(); ++y) {
            ++tested;
            passed += test(x, y, false) ? 1 : 0;
        }
    }
    fmt::println("{}/{} passed. ({:.2f}% failed)", passed, tested, (float)(tested - passed)/tested * 100.f);

    return 0;
}
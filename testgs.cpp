#include <chrono>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <functional>
#include <random>
#include "grid_structure.hpp"
#include <ranges>
#include <tuple>
#include <vector>


int test_grid_access_performance() {
    using gs_type = grid_structure<3>;
    static constexpr unsigned resx = 4096 * 2;
    static constexpr unsigned resy = 4096 * 2;
    static constexpr unsigned border = 4;
    static constexpr size_t TEST_CNT = 5;

    gs_type gs(resx / gs_type::gw, resy / gs_type::gh);
    fmt::println("Test with {} x {} grid ({} x {} areas of {}x{} = {} elements per area).",
        gs.width(), gs.height(), gs.areas_width_, gs.areas_height_, gs_type::gw, gs_type::gh, gs_type::area_size);
    fmt::println("Data amount of image: {:.1f}MB", sizeof(float) * gs.size() / (float)(1<<20) );
    fmt::println("Data amount of area : {:.1f}KB", sizeof(float) * gs_type::area_size / (float)(1<<10) );
    fmt::println("Using {0}x{0} matrix with {1} + 2 memory accesses per pixel operation. Run test {2} times.",
        2*border+1, (2*border+1)*(2*border+1), TEST_CNT);
    std::vector<float> fgrid1(gs.size());
    std::vector<float> fgrid2(gs.size());
    std::vector<float> flinear1(gs.size());
    std::vector<float> flinear2(gs.size());
    auto lin_coord_to_offset = [w = gs.width()](unsigned x, unsigned y) {
        return y * w + x;
    };
    auto mk_randomizer = [](float max){
        std::mt19937_64 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(0, max);
        return [=]() mutable { return dist(gen); };
    };
    auto fgen = mk_randomizer(10.0f);
    for(auto& e: fgrid1)
        e = fgen();
    for(auto e: {&fgrid2, &flinear1, &flinear2})
        std::copy(fgrid1.begin(), fgrid1.end(), e->begin());

    // grid access
    auto ga = [&](std::vector<float> & v, unsigned const & x, unsigned const & y) -> float& {
        //return v[gs.coord_to_offset(x, y)];
        return gs.acc(v, x, y);
    };
    // linear access
    auto la = [&](std::vector<float>& v, unsigned const & x, unsigned const & y) -> float& {
        return v[lin_coord_to_offset(x, y)];
    };
    auto perform_test = [](size_t test_cnt, unsigned width, unsigned height,
        std::vector<float>& grid_a, std::vector<float>& grid_b, unsigned border,
        auto&& acc, std::string_view desc) -> float
    {
        std::vector<float>* src = &grid_a;
        std::vector<float>* tgt = &grid_b;
        auto start = std::chrono::high_resolution_clock::now();
        // blur effect - blends brightness towards the average of neighbors
        for(size_t i = 0; i < test_cnt; ++i) {
            for(unsigned y = border; y < height - border; ++y) {
                for(unsigned x = border; x < width - border; ++x) {
                    float avg = 0.f;
                    for(unsigned yb = y - border; yb < y + border + 1; ++yb) {
                        for(unsigned xb = x - border; xb < x + border + 1; ++xb) {
                            avg += acc(*src, xb, yb);
                        }
                    }
                    avg /= (2*border+1) * (2*border+1);
                    acc(*tgt, x, y) = acc(*src, x, y) + (avg - acc(*src, x, y)) * 0.2f ;
                }
            }
            std::swap(src, tgt);
        }
        auto stop = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> s = stop - start;
        float duration = s.count();
        fmt::println("Test {:<20}: {:6.3f}s", desc, duration);
        return duration;
    };

    float duration_grid_s = perform_test(TEST_CNT, gs.width(), gs.height(), fgrid1, fgrid2, border, ga, "with grid access");
    float duration_linear_s = perform_test(TEST_CNT, gs.width(), gs.height(), flinear1, flinear2, border, la, "with linear access");;
    fmt::println("grid : linear = {:.2}:1", duration_grid_s / duration_linear_s);
    return 0;
}

int test_conversion_functions() {

    unsigned coords[][2] = {{0, 0}, {1, 0}, {0, 1}, {7, 7,}, {8, 0}, {0, 8}, {1, 8}, {8, 8},
    {9, 0}, {0, 9}, {9, 9}};

    static constexpr grid_structure<> gs(7, 3);

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
    static constexpr size_t NUM_RANDOM_TESTS = 100000;
    for(auto i : std::views::iota(1) | std::views::take(NUM_RANDOM_TESTS)) {
        ++tested;
        passed += test(rnd_x(), rnd_y()) ? 1 : 0;
    }

    for(unsigned x = 0; x < gs.width(); ++x) {
        for(unsigned y = 0; y < gs.height(); ++y) {
            ++tested;
            passed += test(x, y, false) ? 1 : 0;
        }
    }
    fmt::println("{}/{} passed. ({:.2f}% failed)", passed, tested, (float)(tested - passed)/tested * 100.f);

    return tested == passed ? 0 : 1;

}

int main(int argc, char const *argv[])
{
    int ret = test_conversion_functions();

    test_grid_access_performance();

    auto o = grid_structure<3>(5).coord_to_offset(9,12);
    unsigned const x = 9, y = 12, areas_width_ = 5;
    static constexpr auto o2 = (y * 8 * areas_width_) + ((y % 8) * 8) + x * 8 + (x % 8);


    return ret;
}
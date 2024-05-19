#pragma once

#include <cstddef>
#include <tuple>

/**
 * @brief idea: 2D-Gridstructure which is organized in small areas
 * to keep close points also close in memory. I.e. if you need to
 * access the left, right, bottom, and top neighbours these should
 * be close in memory, ideally in the cache.
 * /-gw-\
 * ........########........########........ \
 * ........########........########........  gh
 * ........########........########........ /
 * ########........########........########
 * ########...P....########........########
 * ########........########........########
 * ........########........########........
 * ........########........########........
 * ........########........########........
 * P's position in ,memory:
 * floor(py / gh) * width_areas * gw * gh + floor(px / gw) * gw * gh + mod(py, gh) * gw + mod(px, gw)
 * Use 2^x for gw and gh makes division a simple bit shift and mod a simple bit mask
 * e.g. gw = gh = 8 and P(12, 9): 0x0C, 0x09
 * area starts at (0x09 >> 3) * 64 * width_areas + (0x0C >> 3) * 64  + (0x09 & 0x07) * 8 + (0x0C & 0x07)
                 * (0x09 & 0xfa << 3)      * width_areas + (0x0C & 0xfa << 3)      + (0x09 & 0x07 << 3) + (0x0C & 0x07)
 * @tparam     SHIFT_LEFT  the power of 2 (bits) to use for area width and height
 */
template<size_t SHIFT_LEFT = 3>
struct grid_structure {
    static constexpr unsigned shift_left = SHIFT_LEFT;
    static constexpr unsigned gw = 1 << shift_left, gh = 1 << shift_left;
    static constexpr unsigned area_size = gw * gh;
    static constexpr unsigned mask_mod = gw - 1;
    static constexpr unsigned mask_floor = ~mask_mod;

    unsigned const areas_width_, areas_height_;

    constexpr unsigned width() const noexcept { return areas_width_ * gw; }
    constexpr unsigned height() const noexcept { return areas_height_ * gh; }
    constexpr size_t size() const noexcept { return width() * height(); }

    constexpr auto coord_to_offset(unsigned x, unsigned y) const noexcept -> size_t {
        return ((y & mask_floor) << shift_left) * areas_width_
             + ((x & mask_floor) << shift_left)
             + ((y & mask_mod) << shift_left)
             + (x & mask_mod);
    }

    constexpr auto coord_to_offset(auto const & coord) const noexcept -> size_t
    requires requires {
        { std::get<0>(coord) } -> std::convertible_to<unsigned const>;
        { std::get<1>(coord) } -> std::convertible_to<unsigned const>;
    }
    {
        return coord_to_offset(
            std::forward<unsigned const>(std::get<0>(coord)),
            std::forward<unsigned const>(std::get<1>(coord))
            );
    }

    constexpr auto offset_for_area(unsigned area) const noexcept -> size_t
    { return area << (shift_left + shift_left); }

    constexpr auto area_for_offset(size_t off) const noexcept -> unsigned
    { return (off >> (shift_left + shift_left)); }

    constexpr auto offset_to_coord(size_t off) const noexcept -> std::tuple<unsigned, unsigned> {
        auto x1 = (off & mask_mod);
        auto area_nr = area_for_offset(off);
        auto area_rows_before = area_nr / areas_width_;
        auto x = x1 + ((area_nr % areas_width_) << shift_left) ;
        auto y = (area_rows_before << shift_left) + ((off % area_size) >> shift_left);
        return std::make_tuple<unsigned, unsigned>(x, y);
    }

};

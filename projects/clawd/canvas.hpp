// canvas.hpp - shared pixel buffer used by every other clawd header.
// Contract owner: project lead. Do not change signatures without telling the lead.
#pragma once

#include <cstdint>
#include <vector>

namespace clawd {

struct Rgb {
    std::uint8_t r = 0, g = 0, b = 0;
    constexpr bool operator==(const Rgb& o) const { return r == o.r && g == o.g && b == o.b; }
    constexpr bool operator!=(const Rgb& o) const { return !(*this == o); }
};

// One logical pixel. `on == false` means transparent.
struct Pixel {
    Rgb c{};
    bool on = false;
    constexpr bool operator==(const Pixel& o) const {
        return on == o.on && (!on || c == o.c);
    }
    constexpr bool operator!=(const Pixel& o) const { return !(*this == o); }
};

class Canvas {
public:
    Canvas() = default;
    Canvas(int w, int h) : w_(w), h_(h), px_(static_cast<std::size_t>(w) * h) {}

    int width() const { return w_; }
    int height() const { return h_; }

    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }

    // Out-of-range reads return a transparent pixel; out-of-range writes are ignored.
    Pixel get(int x, int y) const { return inside(x, y) ? px_[idx(x, y)] : Pixel{}; }
    void set(int x, int y, Rgb c) {
        if (inside(x, y)) px_[idx(x, y)] = Pixel{c, true};
    }
    void erase(int x, int y) {
        if (inside(x, y)) px_[idx(x, y)] = Pixel{};
    }
    void clear() { for (auto& p : px_) p = Pixel{}; }

    // Copies opaque pixels of `src` onto this canvas with src's (0,0) at (x,y).
    void blit(const Canvas& src, int x, int y) {
        for (int j = 0; j < src.h_; ++j)
            for (int i = 0; i < src.w_; ++i) {
                const Pixel p = src.px_[src.idx(i, j)];
                if (p.on) set(x + i, y + j, p.c);
            }
    }

    bool operator==(const Canvas& o) const { return w_ == o.w_ && h_ == o.h_ && px_ == o.px_; }
    bool operator!=(const Canvas& o) const { return !(*this == o); }

private:
    std::size_t idx(int x, int y) const { return static_cast<std::size_t>(y) * w_ + x; }
    int w_ = 0, h_ = 0;
    std::vector<Pixel> px_;
};

}  // namespace clawd

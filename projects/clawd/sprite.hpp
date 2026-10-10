// sprite.hpp - Clawd, his guitar and small decorative glyphs, drawn on a Canvas.
// All drawing is a pure function of its arguments (no static state, no randomness).
#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

#include "canvas.hpp"

namespace clawd::sprite {

constexpr int kBodyW = 14, kBodyH = 12;  // footprint incl. legs; top-left anchored
constexpr Rgb kOrange{217, 119, 87}, kOrangeDark{176, 86, 58}, kEye{36, 26, 26}, kWhite{245, 240, 230};

enum class Eyes { Open, Closed, Happy };  // Closed = blink line, Happy = ^ ^

struct Pose {
    Eyes eyes = Eyes::Open;
    int armL = 0, armR = 0;  // 0 = down, 1 = out/mid, 2 = raised (wave)
    int legs = 0;            // 0 = both planted, 1 = left lifted, 2 = right lifted
    int squash = 0;          // -1 stretched by 1 row, 0 normal, +1 squashed (breathing)
};

namespace detail {

// Palette letters: '#' orange, 'o' dark orange, 'e' eye, 'w' white, '.' transparent,
// guitar: 'r' red, 'R' dark red, 'q' red highlight, 'k' pickup, 's' string, 'y' gold,
//         'n' neck, 'N' neck shade, 'F' fret, 'h' headstock wood.
constexpr std::size_t slen(const char* s) {
    std::size_t n = 0;
    while (s[n] != '\0') ++n;
    return n;
}

template <std::size_t N>
constexpr bool rowsOk(const char* const (&t)[N], std::size_t w) {
    for (std::size_t i = 0; i < N; ++i)
        if (slen(t[i]) != w) return false;
    return true;
}

inline bool colorOf(char ch, Rgb& out) {
    switch (ch) {
        case '#': out = kOrange; return true;
        case 'o': out = kOrangeDark; return true;
        case 'e': out = kEye; return true;
        case 'w': out = kWhite; return true;
        case 'r': out = Rgb{178, 34, 40}; return true;
        case 'R': out = Rgb{104, 18, 26}; return true;
        case 'q': out = Rgb{232, 96, 88}; return true;
        case 'k': out = Rgb{28, 28, 34}; return true;
        case 's': out = Rgb{205, 205, 215}; return true;
        case 'y': out = Rgb{232, 192, 72}; return true;
        case 'n': out = Rgb{226, 190, 140}; return true;
        case 'N': out = Rgb{168, 124, 80}; return true;
        case 'F': out = Rgb{250, 232, 200}; return true;
        case 'h': out = Rgb{92, 56, 36}; return true;
        default: return false;
    }
}

template <std::size_t N>
void blitRows(Canvas& c, int x, int y, const char* const (&t)[N], bool mirror = false) {
    for (std::size_t j = 0; j < N; ++j) {
        const std::size_t w = slen(t[j]);
        for (std::size_t i = 0; i < w; ++i) {
            Rgb col;
            const std::size_t src = mirror ? w - 1 - i : i;
            if (colorOf(t[j][src], col)) c.set(x + static_cast<int>(i), y + static_cast<int>(j), col);
        }
    }
}

// ---- Clawd body (14 wide, 8 rows; row 5 is duplicated / dropped for squash) ----
inline constexpr const char* kBodyRows[] = {
    "...########...",
    "..##########..",
    "..##########..",
    "..##########..",
    "..##########..",
    "..##########..",
    "..##########..",
    "..oooooooooo..",
};
constexpr int kBodyRowsN = 8;
constexpr int kBodyStretchRow = 5;

// Eyes: 2 rows, 14 wide, placed at body-top + 2.
inline constexpr const char* kEyesOpen[] = {"....ee..ee....", "....ee..ee...."};
inline constexpr const char* kEyesClosed[] = {"..............", "....ee..ee...."};
inline constexpr const char* kEyesHappy[] = {"....e....e....", "...e.e..e.e..."};

// Legs: 2 wide.
inline constexpr const char* kLegDown[] = {"##", "##", "oo"};
inline constexpr const char* kLegUp[] = {"##", "oo"};

// Arms (left arm; right is mirrored): 2 wide, 7 rows anchored at y.
inline constexpr const char* kArmDown[] = {"..", "..", "..", "..", "##", "##", "oo"};
inline constexpr const char* kArmOut[] = {"..", "..", "#.", "##", "oo", "..", ".."};
inline constexpr const char* kArmUp[] = {"oo", "##", "##", "##", "..", "..", ".."};

// ---- Guitar body, anchored at (x+2, y+5), 11 wide x 5 high ----
inline constexpr const char* kGuitarBody[] = {
    "..RRRRRRR..",
    ".RrqrrrrrrR",
    "RsskksskksR",
    ".RrrrrrrrrR",
    "..RRRRRRR..",
};
constexpr int kGuitarBodyX = 2, kGuitarBodyY = 5;

// Right (strumming) arm, 5 wide x 7 high anchored at (x+9, y+1).
inline constexpr const char* kStrum0[] = {"...oo", "...##", "...##", "...##", ".....", ".....", "....."};
inline constexpr const char* kStrum1[] = {".....", ".....", "...##", "...##", "..oo.", ".....", "....."};
inline constexpr const char* kStrum2[] = {".....", ".....", "...##", "...##", "..##.", ".##..", "oo..."};

// ---- Glyphs ----
inline constexpr const char* kNote0[] = {"...#.", "...##", "...#.", "...#.", ".###.", ".###.", "..#.."};
inline constexpr const char* kNote1[] = {".####", ".####", ".#..#", ".#..#", ".#..#", "##.##", "##.##"};
inline constexpr const char* kStar2[] = {"..#..", ".###.", "#####", ".###.", "..#.."};
inline constexpr const char* kThink0[] = {"...#...", ".#.#.#.", "..###..", "#######", "..###..", ".#.#.#.", "...#..."};
inline constexpr const char* kThink1[] = {"#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", ".#...#.", "#.....#"};
inline constexpr const char* kThink2[] = {".......", "..#.#..", "...#...", ".#####.", "...#...", "..#.#..", "......."};
inline constexpr const char* kThink3[] = {"...#...", "..###..", ".#.#.#.", "#######", ".#.#.#.", "..###..", "...#..."};

static_assert(rowsOk(kBodyRows, 14), "body rows must be 14 wide");
static_assert(rowsOk(kEyesOpen, 14) && rowsOk(kEyesClosed, 14) && rowsOk(kEyesHappy, 14), "eye rows");
static_assert(rowsOk(kLegDown, 2) && rowsOk(kLegUp, 2), "leg rows");
static_assert(rowsOk(kArmDown, 2) && rowsOk(kArmOut, 2) && rowsOk(kArmUp, 2), "arm rows");
static_assert(rowsOk(kGuitarBody, 11), "guitar body rows");
static_assert(rowsOk(kStrum0, 5) && rowsOk(kStrum1, 5) && rowsOk(kStrum2, 5), "strum rows");
static_assert(rowsOk(kNote0, 5) && rowsOk(kNote1, 5) && rowsOk(kStar2, 5), "glyph rows");
static_assert(rowsOk(kThink0, 7) && rowsOk(kThink1, 7) && rowsOk(kThink2, 7) && rowsOk(kThink3, 7), "think rows");

inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Draws legs, body and eyes. Arms are drawn separately.
inline void drawBase(Canvas& c, int x, int y, const Pose& p) {
    const int sq = clampi(p.squash, -1, 1);
    const int h = kBodyRowsN + 1 - 1 - sq;  // +1 squashed -> 7 rows, -1 stretched -> 9 rows
    const int top = 9 - h;                  // body bottom row is always y+8

    // legs (rows 9..11)
    const int ly = 9;
    if (p.legs == 1) blitRows(c, x + 3, y + ly, kLegUp);  else blitRows(c, x + 4, y + ly, kLegDown);
    if (p.legs == 2) blitRows(c, x + 9, y + ly, kLegUp);  else blitRows(c, x + 8, y + ly, kLegDown);

    // body
    for (int i = 0; i < h; ++i) {
        int src = i;
        if (h > kBodyRowsN) src = (i <= kBodyStretchRow) ? i : i - 1;
        else if (h < kBodyRowsN) src = (i < kBodyStretchRow) ? i : i + 1;
        const char* const row[] = {kBodyRows[src]};
        blitRows(c, x, y + top + i, row);
    }

    // eyes
    switch (p.eyes) {
        case Eyes::Open: blitRows(c, x, y + top + 2, kEyesOpen); break;
        case Eyes::Closed: blitRows(c, x, y + top + 2, kEyesClosed); break;
        case Eyes::Happy: blitRows(c, x, y + top + 2, kEyesHappy); break;
    }
}

inline void drawArm(Canvas& c, int x, int y, int mode, bool right) {
    const int ax = right ? x + 12 : x;
    switch (clampi(mode, 0, 2)) {
        case 0: blitRows(c, ax, y, kArmDown, right); break;
        case 1: blitRows(c, ax, y, kArmOut, right); break;
        default: blitRows(c, ax, y, kArmUp, right); break;
    }
}

}  // namespace detail

inline void drawClawd(Canvas& c, int x, int y, const Pose& p) {
    detail::drawBase(c, x, y, p);
    detail::drawArm(c, x, y, p.armL, false);
    detail::drawArm(c, x, y, p.armR, true);
}

inline void drawClawdGuitar(Canvas& c, int x, int y, const Pose& p, int strum) {
    detail::drawBase(c, x, y, p);

    // neck: diagonal from the body's upper-left corner up to the headstock
    for (int t = 0; t < 10; ++t) {
        const int nx = x + 2 - t, ny = y + 5 - t;
        Rgb col;
        detail::colorOf((t % 3 == 1) ? 'F' : 'n', col);
        c.set(nx, ny, col);
        detail::colorOf('N', col);
        c.set(nx + 1, ny, col);
    }
    {
        Rgb col;
        detail::colorOf('y', col);
        c.set(x - 9, y - 5, col);  // tuning pegs
        detail::colorOf('h', col);
        c.set(x - 8, y - 5, col);
        c.set(x - 7, y - 5, col);
        c.set(x - 8, y - 4, col);
    }
    detail::blitRows(c, x + detail::kGuitarBodyX, y + detail::kGuitarBodyY, detail::kGuitarBody);

    // left hand on the neck, right hand strumming
    detail::blitRows(c, x, y, detail::kArmOut);
    switch (detail::clampi(strum, 0, 2)) {
        case 0: detail::blitRows(c, x + 9, y + 1, detail::kStrum0); break;
        case 1: detail::blitRows(c, x + 9, y + 1, detail::kStrum1); break;
        default: detail::blitRows(c, x + 9, y + 1, detail::kStrum2); break;
    }
}

inline void drawNote(Canvas& c, int x, int y, Rgb color, int kind) {
    const int ox = x - 2, oy = y - 3;
    const char* const* t = (kind == 0) ? detail::kNote0 : detail::kNote1;
    for (int j = 0; j < 7; ++j)
        for (int i = 0; i < 5; ++i)
            if (t[j][i] == '#') c.set(ox + i, oy + j, color);
}

inline void drawSpark(Canvas& c, int x, int y, Rgb color, int size) {
    if (size <= 0) {
        c.set(x, y, color);
    } else if (size == 1) {
        c.set(x, y, color);
        c.set(x - 1, y, color);
        c.set(x + 1, y, color);
        c.set(x, y - 1, color);
        c.set(x, y + 1, color);
    } else {
        for (int j = 0; j < 5; ++j)
            for (int i = 0; i < 5; ++i)
                if (detail::kStar2[j][i] == '#') c.set(x - 2 + i, y - 2 + j, color);
    }
}

inline void drawThinkMark(Canvas& c, int x, int y, Rgb color, int phase) {
    const char* const* t = detail::kThink0;
    switch (((phase % 4) + 4) % 4) {
        case 0: t = detail::kThink0; break;
        case 1: t = detail::kThink1; break;
        case 2: t = detail::kThink2; break;
        default: t = detail::kThink3; break;
    }
    for (int j = 0; j < 7; ++j)
        for (int i = 0; i < 7; ++i)
            if (t[j][i] == '#') c.set(x - 3 + i, y - 3 + j, color);
}

inline int selftest() {
    int fails = 0;
    auto fail = [&](const char* msg) {
        std::cerr << "sprite selftest FAIL: " << msg << "\n";
        ++fails;
    };
    auto count = [](const Canvas& c) {
        int n = 0;
        for (int j = 0; j < c.height(); ++j)
            for (int i = 0; i < c.width(); ++i)
                if (c.get(i, j).on) ++n;
        return n;
    };

    // runtime row-width checks (mirror of the static_asserts)
    if (!detail::rowsOk(detail::kBodyRows, 14)) fail("body row widths");
    if (!detail::rowsOk(detail::kGuitarBody, 11)) fail("guitar row widths");
    if (!detail::rowsOk(detail::kStrum0, 5) || !detail::rowsOk(detail::kStrum1, 5) ||
        !detail::rowsOk(detail::kStrum2, 5)) fail("strum row widths");
    if (!detail::rowsOk(detail::kThink0, 7) || !detail::rowsOk(detail::kThink3, 7)) fail("think row widths");

    // eye states
    Canvas e[3];
    const Eyes kinds[3] = {Eyes::Open, Eyes::Closed, Eyes::Happy};
    for (int k = 0; k < 3; ++k) {
        e[k] = Canvas(kBodyW, kBodyH);
        Pose p;
        p.eyes = kinds[k];
        drawClawd(e[k], 0, 0, p);
        if (count(e[k]) == 0) fail("drawClawd drew nothing");
    }
    if (e[0] == e[1]) fail("Open and Closed eyes look identical");
    if (e[0] == e[2]) fail("Open and Happy eyes look identical");

    // body stays inside its footprint for every pose variation
    for (int sq = -1; sq <= 1; ++sq)
        for (int al = 0; al <= 2; ++al)
            for (int lg = 0; lg <= 2; ++lg) {
                Canvas big(40, 30);
                Pose p;
                p.squash = sq; p.armL = al; p.armR = 2 - al; p.legs = lg;
                drawClawd(big, 10, 8, p);
                for (int j = 0; j < 30; ++j)
                    for (int i = 0; i < 40; ++i)
                        if (big.get(i, j).on &&
                            (i < 10 || i >= 10 + kBodyW || j < 8 || j >= 8 + kBodyH))
                            fail("clawd pixel outside footprint");
            }

    // strum poses pairwise different
    Canvas s[3];
    for (int k = 0; k < 3; ++k) {
        s[k] = Canvas(40, 30);
        drawClawdGuitar(s[k], 14, 8, Pose{}, k);
        if (count(s[k]) == 0) fail("drawClawdGuitar drew nothing");
    }
    if (s[0] == s[1] || s[0] == s[2] || s[1] == s[2]) fail("strum poses not pairwise different");

    // edge safety: partially or fully outside, and tiny/empty canvases
    {
        Canvas small(10, 10);
        Pose p;
        const int pos[] = {-50, -14, -7, 0, 5, 9, 10, 50};
        for (int xx : pos)
            for (int yy : pos) {
                drawClawd(small, xx, yy, p);
                for (int k = 0; k < 3; ++k) drawClawdGuitar(small, xx, yy, p, k);
                drawNote(small, xx, yy, kWhite, 0);
                drawNote(small, xx, yy, kWhite, 1);
                for (int sz = 0; sz < 3; ++sz) drawSpark(small, xx, yy, kWhite, sz);
                for (int ph = -1; ph < 5; ++ph) drawThinkMark(small, xx, yy, kWhite, ph);
            }
        Canvas empty;
        drawClawd(empty, 0, 0, p);
        drawClawdGuitar(empty, 0, 0, p, 1);
    }

    // glyphs draw something and stay within their bounding boxes
    for (int kind = 0; kind < 2; ++kind) {
        Canvas g(20, 20);
        drawNote(g, 10, 10, kWhite, kind);
        if (count(g) == 0) fail("note empty");
    }
    for (int sz = 0; sz < 3; ++sz) {
        Canvas g(20, 20);
        drawSpark(g, 10, 10, kWhite, sz);
        if (count(g) == 0) fail("spark empty");
    }
    for (int ph = 0; ph < 4; ++ph) {
        Canvas g(20, 20);
        drawThinkMark(g, 10, 10, kWhite, ph);
        if (count(g) == 0) fail("think mark empty");
    }
    return fails;
}

}  // namespace clawd::sprite

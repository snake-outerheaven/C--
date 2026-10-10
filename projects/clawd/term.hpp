// term.hpp - terminal session handling and diff-based frame presenter.
// Only the C++ standard library, plus one platform section for console setup.
#pragma once

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "canvas.hpp"

// ---------------------------------------------------------------------------
// Platform section (the only place OS headers are allowed).
// ---------------------------------------------------------------------------
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <io.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace clawd {
namespace detail {

#ifdef _WIN32
inline bool platformIsTty() { return _isatty(_fileno(stdout)) != 0; }

inline bool envNonEmpty(const char* name) {
    char* buf = nullptr;
    std::size_t len = 0;
    if (_dupenv_s(&buf, &len, name) != 0 || buf == nullptr) return false;
    const bool nonEmpty = buf[0] != '\0';
    std::free(buf);
    return nonEmpty;
}

inline bool platformSize(int& cols, int& rows) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) return false;
    const int c = info.srWindow.Right - info.srWindow.Left + 1;
    const int r = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (c <= 0 || r <= 0) return false;
    cols = c;
    rows = r;
    return true;
}

struct ConsoleSaved {
    DWORD mode = 0;
    UINT cp = 0;
    bool haveMode = false;
    bool haveCp = false;
};

inline void platformSetup(ConsoleSaved& s) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && h != nullptr && GetConsoleMode(h, &mode)) {
        s.mode = mode;
        s.haveMode = true;
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    const UINT cp = GetConsoleOutputCP();
    if (cp != 0) {
        s.cp = cp;
        s.haveCp = true;
        SetConsoleOutputCP(CP_UTF8);
    }
}

inline void platformRestore(const ConsoleSaved& s) {
    if (s.haveMode) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE && h != nullptr) SetConsoleMode(h, s.mode);
    }
    if (s.haveCp) SetConsoleOutputCP(s.cp);
}
#else
inline bool platformIsTty() { return ::isatty(STDOUT_FILENO) != 0; }

inline bool envNonEmpty(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0';
}

inline bool platformSize(int& cols, int& rows) {
    struct winsize ws;
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) return false;
    if (ws.ws_col == 0 || ws.ws_row == 0) return false;
    cols = static_cast<int>(ws.ws_col);
    rows = static_cast<int>(ws.ws_row);
    return true;
}

struct ConsoleSaved {};
inline void platformSetup(ConsoleSaved&) {}
inline void platformRestore(const ConsoleSaved&) {}
#endif

// ---------------------------------------------------------------------------
// Session state shared by Terminal, the SIGINT handler and the exit hooks.
// ---------------------------------------------------------------------------
struct SessionState {
    bool active = false;
    bool altScreen = false;
    bool hooksInstalled = false;
    ConsoleSaved saved;
};

inline SessionState& session() {
    static SessionState s;
    return s;
}

inline volatile std::sig_atomic_t& interruptFlag() {
    static volatile std::sig_atomic_t f = 0;
    return f;
}

inline void sigintHandler(int) { interruptFlag() = 1; }

inline void restoreSession() {
    SessionState& s = session();
    if (!s.active) return;
    s.active = false;
    std::string seq = "\x1b[0m\x1b[?25h";
    if (s.altScreen) seq += "\x1b[?1049l";
    std::fwrite(seq.data(), 1, seq.size(), stdout);
    std::fflush(stdout);
    platformRestore(s.saved);
}

inline void atexitHook() { restoreSession(); }

inline void terminateHook() {
    restoreSession();
    std::abort();
}

// ---------------------------------------------------------------------------
// Glyph / cell model used by Presenter.
// ---------------------------------------------------------------------------
// Cell glyph codes in half-block mode: ' ' empty, 'U' upper half, 'L' lower half, 'F' full.
// In ASCII mode the code is the ramp character itself.
struct Cell {
    char g = ' ';
    bool hasFg = false;
    bool hasBg = false;
    Rgb fg{};
    Rgb bg{};
    bool operator==(const Cell& o) const {
        return g == o.g && hasFg == o.hasFg && hasBg == o.hasBg && fg == o.fg && bg == o.bg;
    }
    bool operator!=(const Cell& o) const { return !(*this == o); }
};

inline const char* rampChars() { return " .:-=+*#%@"; }
inline int rampSize() { return 10; }

inline int luminance(const Pixel& p) {
    if (!p.on) return 0;
    return (299 * static_cast<int>(p.c.r) + 587 * static_cast<int>(p.c.g) +
            114 * static_cast<int>(p.c.b)) / 1000;
}

inline std::string glyphFor(char g, bool ascii) {
    if (ascii) return std::string(1, g);
    switch (g) {
        case 'U': return "\xE2\x96\x80";  // U+2580 upper half block
        case 'L': return "\xE2\x96\x84";  // U+2584 lower half block
        case 'F': return "\xE2\x96\x88";  // U+2588 full block
        default: return " ";
    }
}

inline Canvas scaled(const Canvas& src, int s) {
    if (s <= 1) return src;
    Canvas out(src.width() * s, src.height() * s);
    for (int y = 0; y < out.height(); ++y)
        for (int x = 0; x < out.width(); ++x) {
            const Pixel p = src.get(x / s, y / s);
            if (p.on) out.set(x, y, p.c);
        }
    return out;
}

// Builds the cell grid of an (already scaled) canvas: width x ceil(height/2).
inline std::vector<Cell> buildGrid(const Canvas& c, bool ascii, bool color, int& cols, int& rows) {
    cols = c.width();
    rows = (c.height() + 1) / 2;
    std::vector<Cell> grid(static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows > 0 ? rows : 0));
    for (int cy = 0; cy < rows; ++cy) {
        for (int x = 0; x < cols; ++x) {
            const Pixel top = c.get(x, cy * 2);
            const Pixel bot = c.get(x, cy * 2 + 1);
            Cell cell;
            if (ascii) {
                const int lum = (luminance(top) + luminance(bot)) / 2;
                const int idx = lum * (rampSize() - 1) / 255;
                cell.g = rampChars()[idx];
                if (color && (top.on || bot.on)) {
                    cell.hasFg = true;
                    if (top.on && bot.on) {
                        cell.fg.r = static_cast<std::uint8_t>((top.c.r + bot.c.r) / 2);
                        cell.fg.g = static_cast<std::uint8_t>((top.c.g + bot.c.g) / 2);
                        cell.fg.b = static_cast<std::uint8_t>((top.c.b + bot.c.b) / 2);
                    } else {
                        cell.fg = top.on ? top.c : bot.c;
                    }
                }
            } else if (!top.on && !bot.on) {
                cell.g = ' ';
            } else if (top.on && !bot.on) {
                cell.g = 'U';
                if (color) { cell.hasFg = true; cell.fg = top.c; }
            } else if (!top.on && bot.on) {
                cell.g = 'L';
                if (color) { cell.hasFg = true; cell.fg = bot.c; }
            } else if (color) {
                cell.g = 'U';
                cell.hasFg = true; cell.fg = top.c;
                cell.hasBg = true; cell.bg = bot.c;
            } else {
                cell.g = 'F';
            }
            grid[static_cast<std::size_t>(cy) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(x)] = cell;
        }
    }
    return grid;
}

inline void appendInt(std::string& s, int v) { s += std::to_string(v); }

inline void appendMove(std::string& s, int row, int col) {
    s += "\x1b[";
    appendInt(s, row + 1);
    s += ';';
    appendInt(s, col + 1);
    s += 'H';
}

inline void appendRgb(std::string& s, bool fg, const Rgb& c) {
    s += fg ? "38;2;" : "48;2;";
    appendInt(s, c.r); s += ';';
    appendInt(s, c.g); s += ';';
    appendInt(s, c.b);
}

// Active SGR state while emitting.
struct Sgr {
    bool hasFg = false;
    bool hasBg = false;
    Rgb fg{};
    Rgb bg{};
    bool isDefault() const { return !hasFg && !hasBg; }
};

inline void applySgr(std::string& out, Sgr& cur, const Cell& want) {
    const bool fgSame = (cur.hasFg == want.hasFg) && (!want.hasFg || cur.fg == want.fg);
    const bool bgSame = (cur.hasBg == want.hasBg) && (!want.hasBg || cur.bg == want.bg);
    if (fgSame && bgSame) return;
    const bool needReset = (cur.hasFg && !want.hasFg) || (cur.hasBg && !want.hasBg);
    std::string params;
    if (needReset) {
        params = "0";
        cur = Sgr{};
    }
    if (want.hasFg && (!cur.hasFg || cur.fg != want.fg)) {
        if (!params.empty()) params += ';';
        appendRgb(params, true, want.fg);
        cur.hasFg = true;
        cur.fg = want.fg;
    }
    if (want.hasBg && (!cur.hasBg || cur.bg != want.bg)) {
        if (!params.empty()) params += ';';
        appendRgb(params, false, want.bg);
        cur.hasBg = true;
        cur.bg = want.bg;
    }
    out += "\x1b[";
    out += params;
    out += 'm';
}

}  // namespace detail

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
inline bool stdoutIsTty() { return detail::platformIsTty(); }

inline bool colorAllowed() { return stdoutIsTty() && !detail::envNonEmpty("NO_COLOR"); }

inline void terminalSize(int& cols, int& rows) {
    int c = 0, r = 0;
    if (detail::platformSize(c, r)) {
        cols = c;
        rows = r;
    } else {
        cols = 80;
        rows = 24;
    }
}

class Terminal {
 public:
    explicit Terminal(bool altScreen = true) {
        detail::SessionState& s = detail::session();
        if (s.active) return;  // only one live session at a time
        owner_ = true;
        s.active = true;
        s.altScreen = altScreen;
        detail::platformSetup(s.saved);
        if (!s.hooksInstalled) {
            s.hooksInstalled = true;
            std::atexit(detail::atexitHook);
            std::set_terminate(detail::terminateHook);
        }
        std::signal(SIGINT, detail::sigintHandler);
        if (altScreen) {
            const char seq[] = "\x1b[?1049h\x1b[?25l\x1b[2J\x1b[1;1H";
            std::fwrite(seq, 1, sizeof(seq) - 1, stdout);
            std::fflush(stdout);
        }
    }
    ~Terminal() { restore(); }
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    void restore() {
        if (!owner_) return;
        owner_ = false;
        detail::restoreSession();
    }

    static bool interrupted() { return detail::interruptFlag() != 0; }

 private:
    bool owner_ = false;
};

struct PresenterOptions {
    bool ascii = false;  // false: half-block (UTF-8); true: pure ASCII luminance ramp
    bool color = true;   // emit 24-bit SGR color sequences
    int scale = 1;       // each canvas pixel becomes scale x scale pixels
};

class Presenter {
 public:
    explicit Presenter(PresenterOptions o = {}) : opt_(o) {
        if (opt_.scale < 1) opt_.scale = 1;
    }

    void present(const Canvas& frame, std::ostream& out) {
        const Canvas big = detail::scaled(frame, opt_.scale);
        int cols = 0, rows = 0;
        const std::vector<detail::Cell> grid = detail::buildGrid(big, opt_.ascii, opt_.color, cols, rows);

        std::string buf;
        detail::Sgr cur;
        bool full = !valid_;
        if (valid_ && cols != prevCols_) {
            buf += "\x1b[2J";
            full = true;
        }
        const int maxRows = rows > prevRows_ ? rows : prevRows_;
        for (int y = 0; y < maxRows; ++y) {
            if (y >= rows) {
                if (!cur.isDefault()) { buf += "\x1b[0m"; cur = detail::Sgr{}; }
                detail::appendMove(buf, y, 0);
                buf += "\x1b[2K";
                continue;
            }
            const std::size_t base = static_cast<std::size_t>(y) * static_cast<std::size_t>(cols);
            bool inRun = false;
            for (int x = 0; x < cols; ++x) {
                const detail::Cell& cell = grid[base + static_cast<std::size_t>(x)];
                const bool changed = full || y >= prevRows_ || cell != prev_[base + static_cast<std::size_t>(x)];
                if (!changed) {
                    inRun = false;
                    continue;
                }
                if (!inRun) {
                    detail::appendMove(buf, y, x);
                    inRun = true;
                }
                detail::applySgr(buf, cur, cell);
                buf += detail::glyphFor(cell.g, opt_.ascii);
            }
        }
        if (!cur.isDefault()) buf += "\x1b[0m";

        prev_ = grid;
        prevCols_ = cols;
        prevRows_ = rows;
        valid_ = true;

        if (!buf.empty()) out.write(buf.data(), static_cast<std::streamsize>(buf.size()));
        out.flush();
    }

    void reset() {
        valid_ = false;
        prev_.clear();
        prevCols_ = 0;
        prevRows_ = 0;
    }

    std::string snapshot(const Canvas& frame) const {
        const Canvas big = detail::scaled(frame, opt_.scale);
        int cols = 0, rows = 0;
        const std::vector<detail::Cell> grid = detail::buildGrid(big, opt_.ascii, false, cols, rows);
        std::string s;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x)
                s += detail::glyphFor(
                    grid[static_cast<std::size_t>(y) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(x)].g,
                    opt_.ascii);
            s += '\n';
        }
        return s;
    }

 private:
    PresenterOptions opt_;
    std::vector<detail::Cell> prev_;
    int prevCols_ = 0;
    int prevRows_ = 0;
    bool valid_ = false;
};

namespace detail {

inline int countCursorMoves(const std::string& s) {
    int n = 0;
    for (std::size_t i = 0; i + 1 < s.size(); ++i) {
        if (s[i] == '\x1b' && s[i + 1] == '[') {
            std::size_t j = i + 2;
            while (j < s.size() && ((s[j] >= '0' && s[j] <= '9') || s[j] == ';')) ++j;
            if (j < s.size() && s[j] == 'H') ++n;
        }
    }
    return n;
}

inline Canvas selftestCanvas() {
    // 3x3:  R . W / R G . / B . W
    Canvas c(3, 3);
    c.set(0, 0, Rgb{255, 0, 0});
    c.set(2, 0, Rgb{255, 255, 255});
    c.set(0, 1, Rgb{255, 0, 0});
    c.set(1, 1, Rgb{0, 255, 0});
    c.set(0, 2, Rgb{128, 128, 255});
    c.set(2, 2, Rgb{255, 255, 255});
    return c;
}

}  // namespace detail

inline int termSelftest() {
    int failures = 0;
    auto check = [&failures](bool ok, const char* what) {
        if (!ok) {
            std::cerr << "term selftest FAIL: " << what << "\n";
            ++failures;
        }
    };

    const Canvas c = detail::selftestCanvas();

    {
        Presenter p(PresenterOptions{false, true, 1});
        check(p.snapshot(c) == "\xE2\x96\x88\xE2\x96\x84\xE2\x96\x80\n\xE2\x96\x80 \xE2\x96\x80\n",
              "half-block snapshot");
        check(p.snapshot(Canvas()).empty(), "empty canvas snapshot");
    }
    {
        Presenter p(PresenterOptions{true, true, 1});
        const std::string s = p.snapshot(c);
        check(s == "::=\n: =\n", "ascii snapshot");
        bool seven = true;
        for (char ch : s) if (static_cast<unsigned char>(ch) > 0x7F) seven = false;
        check(seven, "ascii snapshot is 7-bit");
    }
    {
        Canvas w(2, 2);
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x) w.set(x, y, Rgb{255, 255, 255});
        Presenter p1(PresenterOptions{true, false, 1});
        Presenter p2(PresenterOptions{true, false, 2});
        check(p1.snapshot(w) == "@@\n", "scale 1 ascii snapshot");
        check(p2.snapshot(w) == "@@@@\n@@@@\n", "scale 2 ascii snapshot doubles size");
        Presenter h1(PresenterOptions{false, false, 1});
        Presenter h2(PresenterOptions{false, false, 2});
        check(h1.snapshot(w).size() == 7 && h2.snapshot(w).size() == 26,
              "scale 2 half-block snapshot size");
    }
    {
        // Diff behaviour (half-block, color).
        Presenter p(PresenterOptions{false, true, 1});
        std::ostringstream o1;
        p.present(c, o1);
        check(!o1.str().empty() && o1.str().find("\x1b[1;1H") == 0, "first present repaints at 1;1");
        check(o1.str().find("38;2;") != std::string::npos, "first present has fg color");

        std::ostringstream o2;
        p.present(c, o2);
        check(o2.str().empty() || o2.str() == "\x1b[0m", "identical frame writes nothing");

        Canvas c2 = c;
        c2.set(1, 0, Rgb{0, 0, 255});
        std::ostringstream o3;
        p.present(c2, o3);
        const std::string s3 = o3.str();
        check(detail::countCursorMoves(s3) == 1, "one-pixel change has exactly one cursor move");
        check(s3.find("\x1b[1;2H") == 0, "one-pixel change targets row 1 col 2");
        check(s3.size() < 80, "one-pixel change is short");

        p.reset();
        std::ostringstream o4;
        p.present(c2, o4);
        check(detail::countCursorMoves(o4.str()) >= 1 && o4.str().size() > s3.size(), "reset repaints fully");

        Canvas shorter(3, 1);
        shorter.set(0, 0, Rgb{1, 2, 3});
        std::ostringstream o5;
        p.present(shorter, o5);
        check(o5.str().find("\x1b[2K") != std::string::npos, "shorter frame clears old rows");
    }
    {
        // No color escapes when color=false.
        for (int a = 0; a < 2; ++a) {
            Presenter p(PresenterOptions{a == 1, false, 1});
            std::ostringstream o;
            p.present(c, o);
            const std::string s = o.str();
            check(s.find("38;2") == std::string::npos && s.find("48;2") == std::string::npos &&
                      s.find("\x1b[0m") == std::string::npos && s.find('m') == std::string::npos,
                  "no SGR when color=false");
            check(p.snapshot(c).find('\x1b') == std::string::npos, "snapshot has no escapes");
        }
    }
    return failures;
}

}  // namespace clawd

// main.cpp - clawd: Claude Code's mascot plays electric guitar in your terminal.
// Only the C++ standard library (plus the console setup section inside term.hpp).
//
// Build:  g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o clawd
//         cl /std:c++17 /EHsc /W4 /permissive- /utf-8 main.cpp
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <fcntl.h>  // _O_BINARY (stdout must not turn "\n" into "\r\n" in --frames mode)
#endif

#include "canvas.hpp"
#include "sprite.hpp"

#include "term.hpp"
#include "anim.hpp"

namespace {

constexpr int kExitUsage = 2;

struct Options {
    bool haveScene = false;
    clawd::SceneId scene = clawd::SceneId::Guitar;
    int bpm = 120;
    int fps = 12;
    int scale = 1;
    bool ascii = false;
    bool once = false;
    bool haveFrames = false;
    long frames = 0;
    unsigned long seed = 1;
    bool selftest = false;
    bool help = false;
};

void usage(std::ostream& os) {
    os << "clawd - Claude Code's mascot plays electric guitar in your terminal\n"
          "\n"
          "usage: clawd [options]\n"
          "\n"
          "  --scene NAME   intro|idle|wave|think|sparks|walk|guitar (default: whole show)\n"
          "  --bpm N        guitar tempo, 40..240 (default 120)\n"
          "  --fps N        frames per second, 1..60 (default 12)\n"
          "  --scale N      pixel scale, 1..4 (default 1)\n"
          "  --ascii        7-bit ASCII output instead of half-block characters\n"
          "  --once         play once and exit (default: loop until Ctrl-C)\n"
          "  --frames N     print N frames to stdout, no terminal control (for pipes/tests)\n"
          "  --seed N       seed for sparks and blinks (default 1)\n"
          "  --selftest     run the built-in self tests\n"
          "  -h, --help     show this help\n";
}

bool parseLong(const char* s, long lo, long hi, long& out) {
    if (s == nullptr || *s == '\0') return false;
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return false;
    if (v < lo || v > hi) return false;
    out = v;
    return true;
}

bool parseSeed(const char* s, unsigned long& out) {
    if (s == nullptr || *s < '0' || *s > '9') return false;  // rejects '-', '+', spaces
    char* end = nullptr;
    errno = 0;
    const unsigned long v = std::strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v > 0xFFFFFFFFul) return false;
    out = v;
    return true;
}

int clampInt(long v, int lo, int hi) { return static_cast<int>(v < lo ? lo : (v > hi ? hi : v)); }

// Returns 0 on success, kExitUsage on error (message already printed).
int parseArgs(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto value = [&](const char*& v) {
            if (i + 1 >= argc) {
                std::cerr << "clawd: option " << a << " needs a value\n";
                return false;
            }
            v = argv[++i];
            return true;
        };
        const char* v = nullptr;
        long n = 0;
        if (a == "-h" || a == "--help") {
            o.help = true;
        } else if (a == "--selftest") {
            o.selftest = true;
        } else if (a == "--ascii") {
            o.ascii = true;
        } else if (a == "--once") {
            o.once = true;
        } else if (a == "--scene") {
            if (!value(v)) return kExitUsage;
            if (!clawd::sceneFromName(v, o.scene)) {
                std::cerr << "clawd: unknown scene '" << v << "' (intro|idle|wave|think|sparks|walk|guitar)\n";
                return kExitUsage;
            }
            o.haveScene = true;
        } else if (a == "--bpm" || a == "--fps" || a == "--scale") {
            if (!value(v)) return kExitUsage;
            if (!parseLong(v, -1000000L, 1000000L, n)) {
                std::cerr << "clawd: invalid number for " << a << ": '" << v << "'\n";
                return kExitUsage;
            }
            if (a == "--bpm") o.bpm = clampInt(n, 40, 240);
            else if (a == "--fps") o.fps = clampInt(n, 1, 60);
            else o.scale = clampInt(n, 1, 4);
        } else if (a == "--frames") {
            if (!value(v)) return kExitUsage;
            if (!parseLong(v, 0, 1000000L, n)) {
                std::cerr << "clawd: invalid number for --frames: '" << v << "' (0..1000000)\n";
                return kExitUsage;
            }
            o.haveFrames = true;
            o.frames = n;
        } else if (a == "--seed") {
            if (!value(v)) return kExitUsage;
            if (!parseSeed(v, o.seed)) {
                std::cerr << "clawd: invalid seed: '" << v << "' (0..4294967295)\n";
                return kExitUsage;
            }
        } else {
            std::cerr << "clawd: unknown option '" << a << "'\n\n";
            usage(std::cerr);
            return kExitUsage;
        }
    }
    return 0;
}

int runSelftest() {
    const int s = clawd::sprite::selftest();
    const int t = clawd::termSelftest();
    const int a = clawd::selftest();
    std::cout << "selftest: sprite " << (s == 0 ? "ok" : "FAILED") << " (" << s << "), term "
              << (t == 0 ? "ok" : "FAILED") << " (" << t << "), anim " << (a == 0 ? "ok" : "FAILED")
              << " (" << a << ")\n";
    const int failures = s + t + a;
    std::cout << (failures == 0 ? "all self tests passed" : "self tests FAILED") << "\n";
    return failures > 0 ? 1 : 0;
}

// Frame source: one scene, or the whole show.
struct Source {
    const clawd::Show& show;
    const Options& opt;
    int length() const { return opt.haveScene ? show.sceneFrames(opt.scene) : show.totalFrames(); }
    clawd::Canvas render(int i) const {
        const int n = length();
        const int f = ((i % n) + n) % n;
        return opt.haveScene ? show.renderScene(opt.scene, f) : show.renderShow(f);
    }
};

void prepareStdout(bool tty) {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
    if (tty) SetConsoleOutputCP(CP_UTF8);
#else
    (void)tty;
#endif
}

int dumpFrames(const Source& src, const clawd::Presenter& pr, long count) {
    prepareStdout(clawd::stdoutIsTty());
    for (long i = 0; i < count; ++i) {
        std::string out = "--- frame " + std::to_string(i) + " ---\n";
        out += pr.snapshot(src.render(static_cast<int>(i)));
        std::fwrite(out.data(), 1, out.size(), stdout);
    }
    std::fflush(stdout);
    return 0;
}

int play(const Source& src, const Options& opt) {
    const int needCols = clawd::kStageW * opt.scale;
    const int needRows = (clawd::kStageH * opt.scale + 1) / 2;
    int cols = 0, rows = 0;
    clawd::terminalSize(cols, rows);
    if (cols < needCols || rows < needRows) {
        std::cerr << "clawd: terminal too small: " << cols << "x" << rows << ", need at least " << needCols
                  << "x" << needRows << (opt.scale > 1 ? " (try a smaller --scale)" : "") << "\n";
        return 1;
    }

    clawd::Terminal term(true);
    clawd::PresenterOptions po;
    po.ascii = opt.ascii;
    po.color = clawd::colorAllowed();
    po.scale = opt.scale;
    clawd::Presenter presenter(po);

    using Clock = std::chrono::steady_clock;
    const auto dt = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / opt.fps));
    const int total = src.length();
    int lastCols = cols, lastRows = rows;
    long frame = 0;
    auto next = Clock::now();

    while (!clawd::Terminal::interrupted()) {
        if (opt.once && frame >= total) break;

        clawd::terminalSize(cols, rows);
        if (cols != lastCols || rows != lastRows) {
            presenter.reset();
            std::cout << "\x1b[2J" << std::flush;
            lastCols = cols;
            lastRows = rows;
        }

        // Padded canvas in unscaled pixels so the stage is centred; presenter stays at 1;1.
        int padW = cols / opt.scale, padH = rows * 2 / opt.scale;
        if (padW < clawd::kStageW) padW = clawd::kStageW;
        if (padH < clawd::kStageH) padH = clawd::kStageH;
        const int ox = (padW - clawd::kStageW) / 2;
        const int oy = ((padH - clawd::kStageH) / 2) & ~1;  // even: keep half-block rows aligned
        clawd::Canvas padded(padW, padH);
        padded.blit(src.render(static_cast<int>(frame)), ox, oy);
        presenter.present(padded, std::cout);

        ++frame;
        next += dt;
        const auto now = Clock::now();
        if (now > next + dt) {
            // More than one frame behind: skip ahead instead of racing to catch up.
            const auto behind = now - next;
            const long skip = static_cast<long>(behind / dt);
            frame += skip;
            next += dt * skip;
        }
        std::this_thread::sleep_until(next);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    if (parseArgs(argc, argv, opt) != 0) return kExitUsage;
    if (opt.help) {
        usage(std::cout);
        return 0;
    }
    if (opt.selftest) return runSelftest();

    clawd::Config cfg;
    cfg.fps = opt.fps;
    cfg.bpm = opt.bpm;
    cfg.seed = static_cast<std::uint32_t>(opt.seed);
    const clawd::Show show(cfg);
    const Source src{show, opt};

    clawd::PresenterOptions po;
    po.ascii = opt.ascii;
    po.color = false;
    po.scale = opt.scale;
    const clawd::Presenter plain(po);

    if (opt.haveFrames) return dumpFrames(src, plain, opt.frames);
    if (!clawd::stdoutIsTty()) {
        std::cerr << "clawd: stdout is not a terminal; printing one pass as plain frames (see --frames)\n";
        return dumpFrames(src, plain, src.length());
    }
    return play(src, opt);
}

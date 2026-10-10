// anim.hpp - scene choreography for the clawd show (pure, deterministic).
//
// renderScene(scene, frame) depends ONLY on (cfg, scene, frame). Randomness is
// derived by hashing (seed, scene, index); all motion is integer math or
// integer lookup tables so output is bit-identical across compilers.
//
// Guitar rhythm clock (all integer):
//   beat   = floor(frame * bpm / (fps * 60))          (== floor(frame / beatFrames))
//   phase  = fractional part of the beat, per mille [0,1000)
//   onset(k) = ceil(k * fps * 60 / bpm)               (first frame of beat k)
//   strum  : every beat starts with strum 2 (downstroke) at its onset frame.
//            even beat: 2 for phase<350, 1 for phase<700, 0 afterwards (down, release to up)
//            odd  beat: 2 for phase<500, 1 afterwards                  (lighter return stroke)
//            final two beats (power chord + stillness): 2 for the first half of the
//            chord beat, then held at 1.
//   body bobs 1px down while phase<500; legs: 1 (even beat) / 2 (odd beat) while
//   phase<500, else 0; eyes Happy on beat%4==0 else Open.
//   Last bar: beat N-2 (N = total beats) is the power chord (ring of 8 size-2 sparks,
//   rock pose); beats N-2 and N-1 are otherwise still (no bob, no foot tap).
#pragma once

#include "canvas.hpp"
#include "sprite.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace clawd {

constexpr int kStageW = 48, kStageH = 32;

enum class SceneId { Intro, Idle, Wave, Think, Sparks, Walk, Guitar };
constexpr int kSceneCount = 7;

inline const char* sceneName(SceneId s) {
    switch (s) {
        case SceneId::Intro: return "intro";
        case SceneId::Idle: return "idle";
        case SceneId::Wave: return "wave";
        case SceneId::Think: return "think";
        case SceneId::Sparks: return "sparks";
        case SceneId::Walk: return "walk";
        case SceneId::Guitar: return "guitar";
    }
    return "idle";
}

inline bool sceneFromName(const std::string& n, SceneId& out) {
    for (int i = 0; i < kSceneCount; ++i) {
        const SceneId s = static_cast<SceneId>(i);
        if (n == sceneName(s)) { out = s; return true; }
    }
    return false;
}

struct Config {
    int fps = 12;
    int bpm = 120;
    std::uint32_t seed = 1;
};

namespace detail {

// ---------------------------------------------------------------- hashing
inline std::uint32_t hash32(std::uint32_t x) {
    x += 0x9E3779B9u;
    x ^= x >> 16; x *= 0x85EBCA6Bu;
    x ^= x >> 13; x *= 0xC2B2AE35u;
    x ^= x >> 16;
    return x;
}
inline std::uint32_t hash3(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    return hash32(hash32(hash32(a) ^ (b * 0x9E3779B1u)) ^ (c * 0x85EBCA77u));
}
inline std::uint32_t uu(int v) { return static_cast<std::uint32_t>(v); }
inline int hmod(std::uint32_t h, int m) { return static_cast<int>(h % static_cast<std::uint32_t>(m)); }

// sin(i * 22.5deg) * 1000, i in [0,16)
inline constexpr int kSin16[16] = {0, 383, 707, 924, 1000, 924, 707, 383,
                                   0, -383, -707, -924, -1000, -924, -707, -383};
inline int sin16(int i) { return kSin16[((i % 16) + 16) % 16]; }
inline int cos16(int i) { return sin16(i + 4); }

inline int floorDiv(int a, int b) {  // b > 0
    int q = a / b;
    if ((a % b) < 0) --q;
    return q;
}
inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline int sanitizeFps(int f) { return f < 1 ? 1 : (f > 240 ? 240 : f); }
inline int sanitizeBpm(int b) { return b < 20 ? 20 : (b > 400 ? 400 : b); }

// ------------------------------------------------------------ guitar clock
inline int guitarBars(const Config& c) {
    const int bars = (8 * c.bpm + 120) / 240;  // ~8 seconds, rounded
    return bars < 2 ? 2 : bars;
}
inline int guitarBeats(const Config& c) { return guitarBars(c) * 4; }
inline int guitarFrames(const Config& c) {
    const long long n = guitarBeats(c);
    return static_cast<int>((n * c.fps * 60 + c.bpm / 2) / c.bpm);
}
// first frame of beat k
inline int beatOnset(const Config& c, int k) {
    const long long num = static_cast<long long>(k) * c.fps * 60;
    return static_cast<int>((num + c.bpm - 1) / c.bpm);
}
inline int beatOf(const Config& c, int frame) {
    return static_cast<int>(static_cast<long long>(frame) * c.bpm / (c.fps * 60));
}
inline int phaseOf(const Config& c, int frame) {  // per mille within the beat
    const long long den = static_cast<long long>(c.fps) * 60;
    const long long rem = static_cast<long long>(frame) * c.bpm % den;
    return static_cast<int>(rem * 1000 / den);
}

// strum value 0 up, 1 mid, 2 down for the guitar scene frame
inline int strumAt(const Config& cfgIn, int frame) {
    Config c = cfgIn;
    c.fps = sanitizeFps(c.fps);
    c.bpm = sanitizeBpm(c.bpm);
    frame = clampi(frame, 0, guitarFrames(c) - 1);
    const int beat = beatOf(c, frame), ph = phaseOf(c, frame);
    const int n = guitarBeats(c);
    if (beat >= n - 2) return (beat == n - 2 && ph < 500) ? 2 : 1;
    if (beat % 2 == 0) return ph < 350 ? 2 : (ph < 700 ? 1 : 0);
    return ph < 500 ? 2 : 1;
}

// ------------------------------------------------------------- particles
struct Particle { int x = 0, y = 0, size = 0; bool alive = false; };

// Ballistic particle computed analytically from (key, age). Velocity units: 1/64 px per frame.
inline Particle particleAt(std::uint32_t key, int age, int life, int ox, int oy, int speedMin, int speedRange) {
    Particle p;
    if (age < 0 || age >= life) return p;
    const int dir = hmod(hash32(key), 16);
    const int speed = speedMin + hmod(hash32(key ^ 0x5bd1e995u), speedRange);
    const int vx = cos16(dir) * speed / 1000;
    const int vy = sin16(dir) * speed / 1000;
    constexpr int g = 6;  // gravity, 1/64 px per frame^2
    p.x = ox + floorDiv(vx * age, 64);
    p.y = oy + floorDiv(2 * vy * age + g * age * age, 128);
    p.size = age * 3 < life ? 2 : (age * 3 < 2 * life ? 1 : 0);
    p.alive = true;
    return p;
}

inline Rgb warmColor(std::uint32_t h) {
    static constexpr Rgb pal[5] = {{255, 200, 60}, {255, 140, 40}, {255, 90, 60}, {255, 230, 150}, {250, 245, 235}};
    return pal[h % 5u];
}
inline Rgb brightColor(int i) {
    static constexpr Rgb pal[6] = {{255, 90, 120}, {90, 220, 255}, {255, 220, 70},
                                   {140, 255, 120}, {200, 130, 255}, {255, 150, 50}};
    return pal[((i % 6) + 6) % 6];
}

// ------------------------------------------------------------------ misc
constexpr int kBodyX = (kStageW - sprite::kBodyW) / 2;      // 17
constexpr int kBodyY = kStageH - 3 - sprite::kBodyH;        // 17

inline int sceneSeconds(SceneId s) {
    switch (s) {
        case SceneId::Intro: return 2;
        case SceneId::Idle: return 4;
        case SceneId::Wave: return 3;
        case SceneId::Think: return 3;
        case SceneId::Sparks: return 3;
        case SceneId::Walk: return 4;
        case SceneId::Guitar: return 8;
    }
    return 1;
}

inline void drawBurst(Canvas& cv, std::uint32_t key, int age, int life, int count, int ox, int oy,
                      int speedMin, int speedRange) {
    for (int i = 0; i < count; ++i) {
        const std::uint32_t k = hash3(key, uu(i), 77u);
        const Particle p = particleAt(k, age, life, ox, oy, speedMin, speedRange);
        if (p.alive) sprite::drawSpark(cv, p.x, p.y, warmColor(hash32(k ^ 0x1234u)), p.size);
    }
}

}  // namespace detail

class Show {
 public:
    explicit Show(Config cfg = {}) : cfg_(cfg) {
        cfg_.fps = detail::sanitizeFps(cfg_.fps);
        cfg_.bpm = detail::sanitizeBpm(cfg_.bpm);
    }
    const Config& config() const { return cfg_; }

    int sceneFrames(SceneId s) const {
        if (s == SceneId::Guitar) return detail::guitarFrames(cfg_);
        return detail::sceneSeconds(s) * cfg_.fps;
    }
    int totalFrames() const {
        int t = 0;
        for (int i = 0; i < kSceneCount; ++i) t += sceneFrames(static_cast<SceneId>(i));
        return t;
    }

    Canvas renderScene(SceneId s, int frame) const {
        frame = detail::clampi(frame, 0, sceneFrames(s) - 1);
        Canvas cv(kStageW, kStageH);
        switch (s) {
            case SceneId::Intro: intro(cv, frame); break;
            case SceneId::Idle: idle(cv, frame); break;
            case SceneId::Wave: wave(cv, frame); break;
            case SceneId::Think: think(cv, frame); break;
            case SceneId::Sparks: sparks(cv, frame); break;
            case SceneId::Walk: walk(cv, frame); break;
            case SceneId::Guitar: guitar(cv, frame); break;
        }
        return cv;
    }

    Canvas renderShow(int frame) const {
        const int total = totalFrames();
        frame = ((frame % total) + total) % total;
        for (int i = 0; i < kSceneCount; ++i) {
            const SceneId s = static_cast<SceneId>(i);
            const int n = sceneFrames(s);
            if (frame < n) return renderScene(s, frame);
            frame -= n;
        }
        return renderScene(SceneId::Guitar, 0);  // unreachable
    }

 private:
    Config cfg_;

    std::uint32_t sceneKey(SceneId s) const {
        return detail::hash3(cfg_.seed, static_cast<std::uint32_t>(s), 0xC1A0u);
    }

    // ------------------------------------------------------------ intro
    void intro(Canvas& cv, int frame) const {
        using namespace detail;
        const int n = sceneFrames(SceneId::Intro);
        sprite::Pose p;
        p.eyes = (frame * 4 < n * 3) ? sprite::Eyes::Closed : sprite::Eyes::Open;
        Canvas full(kStageW, kStageH);
        sprite::drawClawd(full, kBodyX, kBodyY, p);
        const int thr = 60 + frame * 1100 / (n > 1 ? n - 1 : 1);  // per mille
        const std::uint32_t key = sceneKey(SceneId::Intro);
        for (int y = 0; y < kStageH; ++y)
            for (int x = 0; x < kStageW; ++x) {
                const Pixel px = full.get(x, y);
                if (!px.on) continue;
                const bool seedPixel = (x == kBodyX + sprite::kBodyW / 2 && y == kBodyY + sprite::kBodyH / 2);
                if (seedPixel || hmod(hash3(key, uu(x), uu(y)), 1000) < thr) cv.set(x, y, px.c);
            }
    }

    // ------------------------------------------------------------- idle
    void idle(Canvas& cv, int frame) const {
        using namespace detail;
        const int fps = cfg_.fps;
        sprite::Pose p;
        const int br = sin16(frame * 16 / (fps * 2));
        p.squash = br > 600 ? 1 : (br < -600 ? -1 : 0);
        const int sw = sin16(frame * 16 / (fps * 4));
        const int swayX = sw > 800 ? 1 : (sw < -800 ? -1 : 0);
        // blink windows of 1 second; window 2 always blinks (guaranteed), others by hash
        const int win = frame / fps, off = frame % fps;
        const std::uint32_t key = sceneKey(SceneId::Idle);
        const bool blinkWin = (win == 2) || hmod(hash3(key, uu(win), 1u), 3) == 0;
        const int start = hmod(hash3(key, uu(win), 2u), fps > 2 ? fps - 2 : 1);
        if (blinkWin && off >= start && off < start + 2) p.eyes = sprite::Eyes::Closed;
        sprite::drawClawd(cv, kBodyX + swayX, kBodyY, p);
    }

    // ------------------------------------------------------------- wave
    void wave(Canvas& cv, int frame) const {
        sprite::Pose p;
        p.eyes = sprite::Eyes::Happy;
        p.armR = 1 + (frame / 3) % 2;
        sprite::drawClawd(cv, detail::kBodyX, detail::kBodyY, p);
    }

    // ------------------------------------------------------------ think
    void think(Canvas& cv, int frame) const {
        using namespace detail;
        sprite::Pose p;
        sprite::drawClawd(cv, kBodyX, kBodyY, p);
        const int step = std::max(1, cfg_.fps / 3);
        sprite::drawThinkMark(cv, kBodyX + 14, kBodyY - 9, sprite::kWhite, (frame / step) % 4);
        const int dots = (frame / std::max(1, cfg_.fps / 2)) % 4;  // 0..3 trailing dots
        for (int i = 0; i < dots; ++i) {
            const int dx = kBodyX + 11 + i * 2, dy = kBodyY - 1 - i * 2;
            for (int j = 0; j < 2; ++j)
                for (int k = 0; k < 2; ++k) cv.set(dx + j, dy + k, sprite::kWhite);
        }
    }

    // ----------------------------------------------------------- sparks
    void sparks(Canvas& cv, int frame) const {
        using namespace detail;
        const int burstLen = cfg_.fps;           // a burst per second
        const int life = burstLen * 3 / 2;       // overlap with the next burst
        const std::uint32_t key = sceneKey(SceneId::Sparks);
        const int cur = frame / burstLen;
        for (int j = std::max(0, cur - 1); j <= cur; ++j) {
            const int age = frame - j * burstLen;
            const int ox = 14 + hmod(hash3(key, uu(j), 10u), 21);
            const int oy = 8 + hmod(hash3(key, uu(j), 11u), 10);
            drawBurst(cv, hash3(key, uu(j), 12u), age, life, 14, ox, oy, 40, 56);
        }
        sprite::Pose p;
        p.eyes = sprite::Eyes::Happy;
        p.armL = 2; p.armR = 2;
        sprite::drawClawd(cv, kBodyX, kBodyY, p);
    }

    // ------------------------------------------------------------- walk
    void walk(Canvas& cv, int frame) const {
        using namespace detail;
        const int n = sceneFrames(SceneId::Walk);
        const int moveEnd = std::max(1, n * 4 / 5);
        const int f = std::min(frame, moveEnd);
        const int t = f * 1000 / moveEnd;                       // 0..1000
        const int e = t * t / 1000 * (3000 - 2 * t) / 1000;     // smoothstep, per mille
        const int x = kBodyX * e / 1000;
        const bool moving = frame < moveEnd;
        sprite::Pose p;
        int y = kBodyY;
        if (moving) {
            const int ph = (frame / 2) % 2;
            p.legs = 1 + ph;
            p.armL = ph; p.armR = 1 - ph;
            if (ph == 1) y -= 1;
        }
        sprite::drawClawd(cv, x, y, p);
    }

    // ----------------------------------------------------------- guitar
    void guitar(Canvas& cv, int frame) const {
        using namespace detail;
        const Config& c = cfg_;
        const int N = guitarBeats(c);
        const int beat = beatOf(c, frame), ph = phaseOf(c, frame);
        const bool still = beat >= N - 2;
        const int strum = strumAt(c, frame);

        // music notes (one per beat up to the power chord), rise + sinusoidal drift
        const int life = c.fps * 3 / 2;
        for (int k = beat; k >= 0; --k) {
            const int age = frame - beatOnset(c, k);
            if (age >= life) break;
            if (k > N - 3 || age < 0) continue;
            const std::uint32_t h = hash3(c.seed, uu(k), 0x40u);
            const int x0 = (k % 2 == 0 ? 3 : 34) + hmod(h, 4);
            const int drift = 5 * sin16(age * 24 / life + k) / 1000;
            const int y = 13 - age * 12 / life;
            const bool visible = age * 10 < life * 7 || age % 2 == 0;
            if (visible) sprite::drawNote(cv, x0 + drift, y, brightColor(k), k % 2);
        }

        // sparks on strong beats
        const int sparkLife = std::max(4, c.fps * 2 / 3);
        for (int k = beat - (beat % 4); k >= 0 && k > beat - 8; k -= 4) {
            if (k > N - 3) continue;
            const int age = frame - beatOnset(c, k);
            if (age >= sparkLife) break;
            for (int i = 0; i < 3; ++i) {
                const std::uint32_t kk = hash3(c.seed, uu(k), 0x50u + uu(i));
                const Particle p = particleAt(kk, age, sparkLife, 20 + i * 4, 12, 48, 48);
                if (p.alive) sprite::drawSpark(cv, p.x, p.y, warmColor(hash32(kk)), p.size);
            }
        }

        // body
        sprite::Pose pose;
        pose.eyes = (beat % 4 == 0) ? sprite::Eyes::Happy : sprite::Eyes::Open;
        pose.armL = 1;
        int y = kBodyY;
        if (!still && ph < 500) {
            y += 1;
            pose.legs = (beat % 2 == 0) ? 1 : 2;
        }
        if (still) {  // rock pose
            pose.eyes = sprite::Eyes::Happy;
            pose.armR = 2;
            pose.legs = 0;
        }
        sprite::drawClawdGuitar(cv, kBodyX, y, pose, strum);

        // power chord ring
        if (beat >= N - 2) {
            const int age = frame - beatOnset(c, N - 2);
            const int ringLife = std::max(4, c.fps * 2 / 3);
            if (age >= 0 && age < ringLife) {
                const int r = 8 + age * 14 / ringLife;
                for (int i = 0; i < 8; ++i) {
                    const int idx = i * 2;
                    const Rgb col = (i % 2 == 0) ? sprite::kWhite : sprite::kOrange;
                    sprite::drawSpark(cv, 24 + cos16(idx) * r / 1000, 22 + sin16(idx) * r / 1000, col, 2);
                }
                if (age < 2)
                    for (int i = 0; i < 8; ++i)
                        sprite::drawSpark(cv, 24 + cos16(i * 2 + 1) * 5 / 1000 * 1, 22 + sin16(i * 2 + 1) * 5 / 1000,
                                          sprite::kWhite, 1);
            }
        }
    }
};

// ===================================================================== selftest
inline int selftest() {
    int fails = 0;
    auto fail = [&](const std::string& m) { ++fails; std::cerr << "anim selftest FAIL: " << m << "\n"; };
    auto opaque = [](const Canvas& c) {
        int n = 0;
        for (int y = 0; y < c.height(); ++y)
            for (int x = 0; x < c.width(); ++x) n += c.get(x, y).on ? 1 : 0;
        return n;
    };

    Config base;  // 12 fps, 120 bpm, seed 1
    Show show(base);

    // (a) purity
    for (int s = 0; s < kSceneCount; ++s) {
        const SceneId id = static_cast<SceneId>(s);
        const int n = show.sceneFrames(id);
        std::vector<Canvas> fwd;
        for (int f = 0; f < n; ++f) fwd.push_back(show.renderScene(id, f));
        for (int f = n - 1; f >= 0; --f) {
            if (show.renderScene(id, f) != fwd[static_cast<std::size_t>(f)])
                fail(std::string("purity (reverse order) ") + sceneName(id) + " frame " + std::to_string(f));
            if (Show(base).renderScene(id, f) != fwd[static_cast<std::size_t>(f)])
                fail(std::string("purity (fresh Show) ") + sceneName(id) + " frame " + std::to_string(f));
        }
    }

    // (b) seeds matter
    {
        Config c2 = base; c2.seed = 2;
        Show s2(c2);
        bool differs = false;
        for (int s = 0; s < kSceneCount && !differs; ++s) {
            const SceneId id = static_cast<SceneId>(s);
            for (int f = 0; f < show.sceneFrames(id); ++f)
                if (show.renderScene(id, f) != s2.renderScene(id, f)) { differs = true; break; }
        }
        if (!differs) fail("different seeds produce identical shows");
    }

    // (c) non-empty, correct size
    for (int s = 0; s < kSceneCount; ++s) {
        const SceneId id = static_cast<SceneId>(s);
        const int n = show.sceneFrames(id);
        const int fr[3] = {0, n / 2, n - 1};
        for (int f : fr) {
            const Canvas c = show.renderScene(id, f);
            if (c.width() != kStageW || c.height() != kStageH)
                fail(std::string("size ") + sceneName(id));
            if (opaque(c) == 0)
                fail(std::string("empty canvas ") + sceneName(id) + " frame " + std::to_string(f));
        }
    }

    // (d) + (e) guitar sync and length
    {
        const int bpms[3] = {90, 120, 140};
        for (int bpm : bpms) {
            Config c = base; c.bpm = bpm; c.fps = 12;
            const double bf = c.fps * 60.0 / c.bpm;
            const int nf = Show(c).sceneFrames(SceneId::Guitar);
            std::vector<int> onsets;
            int prev = -1;
            for (int f = 0; f < nf; ++f) {
                const int st = detail::strumAt(c, f);
                if (st == 2 && prev != 2) onsets.push_back(f);
                prev = st;
            }
            if (onsets.size() < 8) fail("guitar onsets < 8 at bpm " + std::to_string(bpm));
            for (int k = 0; k < 8 && k < static_cast<int>(onsets.size()); ++k) {
                const int expect = static_cast<int>(k * bf);
                const int d = onsets[static_cast<std::size_t>(k)] - expect;
                if (d < -1 || d > 1)
                    fail("strum onset beat " + std::to_string(k) + " bpm " + std::to_string(bpm) +
                         ": got " + std::to_string(onsets[static_cast<std::size_t>(k)]) + " expected " + std::to_string(expect));
            }
            const double bars = nf / (4 * bf);
            const double rb = static_cast<int>(bars + 0.5);
            if (rb < 2) fail("guitar < 2 bars at bpm " + std::to_string(bpm));
            if (bars - rb > 0.05 || rb - bars > 0.05)
                fail("guitar length not whole bars at bpm " + std::to_string(bpm));
        }
    }

    // (f) show wrap / totals
    {
        int sum = 0;
        for (int s = 0; s < kSceneCount; ++s) sum += show.sceneFrames(static_cast<SceneId>(s));
        if (sum != show.totalFrames()) fail("totalFrames != sum");
        const int t = show.totalFrames();
        if (show.renderShow(0) != show.renderShow(t)) fail("renderShow does not wrap at total");
        if (show.renderShow(5) != show.renderShow(5 + 2 * t)) fail("renderShow does not wrap (2x)");
        if (show.renderShow(t - 1) != show.renderScene(SceneId::Guitar, show.sceneFrames(SceneId::Guitar) - 1))
            fail("renderShow last frame mismatch");
        if (show.renderShow(show.sceneFrames(SceneId::Intro)) != show.renderScene(SceneId::Idle, 0))
            fail("renderShow scene mapping mismatch");
    }

    // names
    for (int s = 0; s < kSceneCount; ++s) {
        SceneId out = SceneId::Intro;
        if (!sceneFromName(sceneName(static_cast<SceneId>(s)), out) || out != static_cast<SceneId>(s))
            fail("scene name round trip");
    }
    {
        SceneId out = SceneId::Intro;
        if (sceneFromName("Guitar", out) || sceneFromName("", out)) fail("sceneFromName should be case-sensitive");
    }

    return fails;
}

}  // namespace clawd

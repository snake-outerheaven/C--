# clawd

[![ci](https://github.com/snake-outerheaven/C--/actions/workflows/ci.yml/badge.svg)](https://github.com/snake-outerheaven/C--/actions/workflows/ci.yml)

**Clawd, the Claude Code mascot, plays electric guitar in your terminal.**
C++17, standard library only, no dependencies.

```
                   :++++++++:
                 ::++  ++  ++::
                 ==++++++++++==
                   ==========
                     ++  ++
                     ..  ..
```

(`clawd --scene idle --frames 1 --ascii`, cropped. In a real terminal it is drawn in color with half-block characters.)

## Features

- Seven scenes: `intro`, `idle`, `wave`, `think`, `sparks`, `walk` and `guitar`, or the whole show in a loop.
- The guitar scene is synced to a tempo (`--bpm`): strums land on the beat and the show ends on a power chord.
- Flicker-free rendering: only changed cells are redrawn (diff-based presenter), in 24-bit color with a half-block pixel grid (two pixels per character cell).
- `--ascii` mode for terminals without UTF-8 or color; `NO_COLOR` is respected.
- Fixed-timestep playback with frame skipping, Ctrl-C restores the terminal (alternate screen, cursor, console mode).
- Centers itself in larger terminals and repaints when the terminal is resized.
- Deterministic: the same flags always produce the same frames (see below).
- Built-in self tests: `clawd --selftest`.

## Usage

```
clawd [options]
```

| Flag | Meaning | Default |
|------|---------|---------|
| `--scene NAME` | `intro`, `idle`, `wave`, `think`, `sparks`, `walk` or `guitar` | whole show |
| `--bpm N` | guitar tempo, clamped to 40..240 | 120 |
| `--fps N` | frames per second, clamped to 1..60 | 12 |
| `--scale N` | pixel scale, clamped to 1..4 | 1 |
| `--ascii` | 7-bit ASCII output instead of half-block characters | off |
| `--once` | play the scene or show once and exit | loop until Ctrl-C |
| `--frames N` | no terminal control: print N frames to stdout, each after a `--- frame i ---` line | off |
| `--seed N` | seed for sparks and blinks (0..4294967295) | 1 |
| `--selftest` | run the self tests of all headers; exit code 1 on failure | |
| `-h`, `--help` | usage | |

An unknown flag or an invalid number prints a message and exits with code 2. If the terminal is smaller than the
frame (48x16 cells at `--scale 1`, multiplied by the scale), clawd says so and exits with code 1. If stdout is not a terminal, one
pass is printed as plain frames, as with `--frames`.

```
clawd                                  # the whole show, looping
clawd --scene guitar --bpm 90 --once   # one guitar solo at 90 bpm
clawd --scale 2 --fps 24               # bigger and smoother
clawd --frames 3 --seed 1 > out.txt    # deterministic text frames
```

## Build

Header-only code plus `main.cpp`; no build system is required.

MSVC (Developer Command Prompt):

```
cl /std:c++17 /EHsc /W4 /permissive- /utf-8 main.cpp
```

g++ or clang++:

```
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o clawd
clang++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o clawd
```

CMake, from the repository root:

```
cmake -S . -B build
cmake --build build --target clawd --config Release
```

On Windows use a terminal with ANSI support (Windows Terminal, or conhost on Windows 10+); clawd enables virtual terminal
processing and UTF-8 output itself and restores the console on exit.

## Architecture

| File | Role |
|------|------|
| `canvas.hpp` | `Rgb`, `Pixel` and `Canvas`: the shared pixel buffer with transparent pixels and `blit` |
| `sprite.hpp` | Clawd's body, eyes, legs, arms, guitar, notes and sparks as pixel art drawn onto a `Canvas` |
| `anim.hpp` | `Show`: the choreography of every scene as a pure function of `(config, scene, frame)` |
| `term.hpp` | `Terminal` (alternate screen, SIGINT, restore on exit), `Presenter` (scaling, half-block or ASCII, cell diff) and the one platform section |
| `main.cpp` | command line, timing loop, centering, `--frames` and `--selftest` |

## Determinism

Frames are computed from integer math and lookup tables only, and randomness comes from hashing `(seed, scene, index)`; there
is no global random state and no floating point in the animation. `renderScene(scene, frame)` therefore always returns the
same pixels, and the text produced by `--frames` was verified to be byte-identical between MSVC and g++ builds.

## License

MIT, see [LICENSE](../../LICENSE).

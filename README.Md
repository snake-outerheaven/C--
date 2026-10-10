# C--

[![ci](https://github.com/snake-outerheaven/C--/actions/workflows/ci.yml/badge.svg)](https://github.com/snake-outerheaven/C--/actions/workflows/ci.yml)

A C++ learning repository: small exercises, study material and a few standalone projects.

## Exercises

Under `exercise/`, one directory per topic:

- `exercise_1_hello_world` - first program
- `exercise_2-variables` - variables and types
- `exercise_3_control` - `break`, `for`, `while`
- `exercise_4_function` - functions and pointers

Every exercise is a target in the root `CMakeLists.txt`.

## Projects

- [`projects/clawd`](projects/clawd/README.md) - a terminal animation of Claude Code's mascot, Clawd, playing electric guitar. C++17, standard library only.

## Building

```sh
cmake -S . -B build
cmake --build build --config Release
```

Build a single target with `--target <name>`, for example `--target clawd`.

## License

MIT, see [LICENSE](LICENSE).

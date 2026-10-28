```
               88                        88
               88                        88                                   ,d
               88                        88                                   88
     ,adPPYba, 88  ,adPPYba, 8b,dPPYba,  88,dPPYba,  ,adPPYYba, 8b,dPPYba,  MM88MMM
    a8P_____88 88 a8P_____88 88P'    °8a 88P'    °8a °°     `Y8 88P'   `°8a   88
    8PP°°°°°°° 88 8PP°°°°°°° 88       d8 88       88 ,adPPPPP88 88       88   88
    °8b,   ,aa 88 °8b,   ,aa 88b,   ,a8° 88       88 88,    ,88 88       88   88,
     `°Ybbd8°' 88  `°Ybbd8°' 88`YbbdP°'  88       88 `°8bbdP°Y8 88       88   °Y888
                             88
                             88                                                °j°m

                                                                 a uci chess engine
```

<div align="center">

# 
[![CI][ci-badge]][ci-link]
[![coverage][coverage-badge]][coverage-link] </br>
[![latest][release-badge]][release-link]
[![commits][commits-badge]][commits-link] 
![commit-activity-badge]</br>
[![LICENSE][license-badge]][license-link]

a work in progress uci chess engine.
</div>

---
I got frustrated about how bad I was at playing chess, so I decided to write myself a chess engine and let the computer do it for me! Not intending to use it against unaware non engine players. This whole project initially started with my old engine [Gambit](https://github.com/looak/Gambit).

Has become more of a obsession lately, and a play ground to try out some C++ I wouldn't write at work.

First commit of Gambit was done on [September the 13th, 2017](https://github.com/looak/Gambit/commit/73ed8535876da5e2de65c7e9c1351b21b536912e). Since then I retired that engine and all my effort is going into elephant.

Taking a test driven approach and implemented compatibilty with OpenBench, of which I have a instance running locally.

Reading a lot on https://talkchess.com and the endless resource https://chessprogramming.org amongst other resources on the internet.
Community on Engine Programmer discord & OpenBench Discord have been very great and helpful.

 Been very inspired by, in no particular order;
- Ciekce's [Polaris](https://github.com/Ciekce/Polaris)
- zzzzz151's [Starzix](https://github.com/zzzzz151/Starzix)
- Analog Hors' [blog](https://analog-hors.github.io/site/home/)
- Sebastian Lague's [Coding Adventures](https://www.youtube.com/@SebastianLague)

## Performance

| Version | moves p/s<br>sngl core | moves p/s<br>mul core|nodes p/s<br>sngl core|[lichess.org][lichess-link] |
|:-------:|:---:|:---:|:---:|:---:|
|[v1.0.0][v1.0.0-link]|~140 million| N/A | ~4.5 million | testing |
|[v0.15.0][v0.15.0-link]|~139 million| N/A | ~5 million | ~2250 elo |
|[v0.13.3][v0.13.3-link]|~18.3 million| N/A | ~1.5 million | N/A |
|[v0.7.0][v0.7.0-link]|~19.5 million| N/A | ~1.65 million | ~1600 elo |
|[v0.6.5][v0.6.5-link]|~16.45 million| N/A | ~1.24 million | ~1500 elo |
|[v0.6.1][v0.6.0-link]|~20 million| N/A | ~1.97 million | N/A |
|v0.5.0|~11 million| N/A | ~1.65 million | ~1100 elo |
|[v0.4.0][v0.4.0-link]|~5 million|~110 million best case | ~600k | ~1350 elo |
|[v0.2.0-alpha.1][v0.2.0-alpha.1-link]| ~4 million | ~35 million best case | ~250k | ~1350 elo |

*moves p/s is perft from the start position, nodes p/s is search from the start position, one thread. v1.0.0 and v0.15.0 are measured with the g++ build OpenBench and the release binaries use (`-O3 -march=native`) on an AMD Ryzen 9 9900X. v0.13.3 on the same CPU, but from a 32-bit build with assertions compiled in, which understates it. Earlier versions on an AMD Ryzen 9 5950X, so the rows aren't directly comparable.*

v0.11.0, the last version before v0.13.3's evaluation and pruning work, reached the low 2000s in bullet, blitz and rapid. Since then PeSTO evaluation gained ~180 Elo and the new pruning ~120 Elo in self-play SPRT tests, see the [v0.13.3 release][v0.13.3-link]. v0.15.0's search and speed work is another +224 Elo over v0.13.7 in self-play, see the [v0.15.0 release][v0.15.0-link]. v1.0.0 adds Lazy SMP: on four threads it's about +200 Elo over v0.15.2 on one, see the [v1.0.0 release][v1.0.0-link].


## Features

* Engine:
    * bitboards with magic bitboard sliding attacks
    * staged legal move generation

* Search:
    * negamax alpha-beta with principal variation search
    * iterative deepening with aspiration windows
    * multi threaded, Lazy SMP: the threads share the transposition table, helper threads search ahead of the main thread at staggered depths
    * transposition table: 8 entries to a 64 byte cache line, each entry one lock-free 64-bit word, every megabyte of `Hash` used
    * draw detection: threefold repetition including the game's earlier positions, fifty move rule
    * quiescence search with delta pruning and static exchange (SEE) pruning
    * check extensions
    * null move pruning with verification, the reduction grows with depth
    * late move reductions
    * late move pruning
    * internal iterative reduction
    * reverse futility pruning
    * futility pruning
    * move ordering: pv & transposition table move, MVV-LVA captures, killer moves, history heuristic, captures that lose material by SEE after the quiet moves

* Time management:
    * soft & hard limits, a new iteration only starts within the soft limit and the hard limit aborts the search
    * increment aware, never plans beyond what is on the clock
    * configurable move overhead for GUI & network latency

* Evaluation:
    * [PeSTO](https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function) material and piece-square tables, tapered between midgame and endgame by game phase, updated incrementally as pieces move
    * mop-up for converting won endgames

* API:
    * "user friendly" cli interface
    * UCI compatible: `go` with depth, nodes, mate, movetime, clock & increments, infinite, ponder and searchmoves, `info hashfull`, `debug on` shows what the helper threads search
    * OpenBench compatible - https://github.com/AndyGrant/OpenBench

## Goals & todo

* evaluation: an NNUE trained on the engine's own games
* search: singular extensions, continuation history, history adjusted reductions, SPSA tuning of the search parameters
* time management: scale the soft limit by best move stability, score drops and nodes spent on the best move
* pondering, the engine already handles `go ponder` but doesn't advertise the `Ponder` option
* a terminal UI
* ~~reach elo 2000~~

* github.io page?

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Downloads

Every [release][release-link] has Windows and Linux binaries of the UCI engine, built for three x86-64 levels and named by the instructions they need. Pick the newest your CPU supports:

| Binary | Needs | CPUs |
|:-------|:------|:-----|
| `x86-64-avx2-bmi2` | AVX2, BMI2 | most from ~2015 on, Intel Haswell & AMD Zen and newer |
| `x86-64-sse42-popcnt` | POPCNT, SSE4.2 | most from ~2009 on |
| `x86-64` | any 64-bit CPU | anything else |

The binaries are statically linked and need nothing else installed. They start in UCI mode, `bench` as an argument runs the OpenBench bench.

### Prerequisites

To build it yourself you need a compiler for [C++20](https://en.cppreference.com/w/cpp/20).

### Installing and Building

Through your choice of means, clone the repository.

git bash example:

```bash
git clone --recursive https://github.com/looak/elephant.git
```

#### Windows

Easiest way to get running on Windows is installing Visual Studio Code and extensions for cmake projects, or Visual Studio Community and opening the initial CMakeLists.txt.

#### Linux

* Create a build directory:

```bash
$ mkdir build
$ cd build
```

* Call Cmake:

```bash
$ cmake ..
```

* Build:

```
$ make
```

## Running Elephant Gambit

Interfacing with elephant can nativly be done through ElephantCLI. As of [v0.4.0][v0.4.0-link] supports [UCI protocol][uci-link] and you can interface it with your Chess GUI of choice. Personally, I have been using [Arena](http://www.playwitharena.de/) & [CuteChess](https://cutechess.com/). Every so often I'll host the engine locally and one can play against it on [lichess.org][lichess-link].

### UCI options

| Option | Default | Range | |
|:-------|:-------:|:-----:|:--|
| `Threads` | 1 | 1–64 | search threads, Lazy SMP |
| `Hash` | 8 | 1–32768 | transposition table size in MB, give it more with more threads and longer time controls |
| `Move Overhead` | 10 | 0–5000 | milliseconds kept in reserve on every move for GUI & network latency, raise it when playing over the internet, e.g. 100 on lichess |

## Running the tests

Either run the output binary `ElephantTest` after build or browse to `.\build\` and execute `ctest`. `ElephantSuite` runs the slower perft suite. The EPD suites (Win at Chess, Arasan, STS) are played over UCI by `test/epd_runner.py`, one engine process per core, in two tiers set in `test/epd_tiers.json`:

| Tier | Runs | Takes | What |
|:--|:--|:--|:--|
| smoke | `test/smoke.sh` (`test\smoke.ps1` from PowerShell) | under a minute | the local sanity check before a merge: unit tests, perft, bench, then WAC with 1 & 4 threads and the first 25 positions of each STS theme, at low node counts |
| extended | `python test/epd_runner.py --engine <ElephantGambit> --tier extended` | minutes, depending on the cores | WAC and all of STS with more budget, plus Arasan 2024 with 1 & 4 threads, CI runs it with perft on pull requests to main |

The EPD suites search a fixed number of nodes per position. With one thread the solved count is the same on every run and every machine, so a change in it means the engine plays differently. Every suite has two levels: below its baseline is a warning that names the drop, below its minimum it fails. A search change can lose a few solves and still gain Elo, that's for SPRT to decide, the minimum only catches drops too large to be anything else. `test/smoke.sh` also warns when the bench differs from the last commit's, expected for a search change, a red flag for a refactor. The runner's `--nodes`, `--depth`, `--movetime`, `--threads` and `--hash` override the settings for experiments. Every position searched is recorded, `test/smoke.sh` compares the run with the nearest ancestor commit's position by position, see [test/RESULTS.md](test/RESULTS.md).

`test\coverage.ps1` (clang-cl) and `test/coverage.sh` (gcc & gcovr) measure what the unit tests reach: they configure `build-cov` with `-DELEPHANT_COVERAGE=ON`, run `ctest -L unit` and write a line, function & branch report to `build-cov/coverage/index.html`. The Coverage workflow (`coverage.yml`, Actions > Coverage > Run workflow) prints the same summary on its page, it runs only by hand. Run on `main` it also updates the coverage badge above: line coverage, in `coverage.json` on the `badges` branch, which holds only the latest.

Changes to the search and evaluation are tested for strength with SPRT on [OpenBench](https://github.com/AndyGrant/OpenBench), which builds the engine with `ob_build/makefile`.

### Bench

OpenBench reads the bench, a node count from a fixed set of searches, from the newest commit's message as `bench <nodes> nodes`. The `hooks/prepare-commit-msg` hook adds it. When a commit changes the engine it builds with `ob_build/makefile` and g++, like OpenBench does, and runs the bench, through WSL on Windows. Other commits reuse the previous bench. Other compilers give different node counts, so the bench always comes from g++.

Install the hook once per clone, from the repository root:

```bash
cp hooks/prepare-commit-msg .git/hooks/ && chmod +x .git/hooks/prepare-commit-msg
```

`ELEPHANT_BENCH=force git commit ...` runs the bench even when no engine file changed, `ELEPHANT_BENCH=skip` leaves it out.

## Versioning

We use [SemVer](http://semver.org/) for versioning, major.minor.patch. The version is written in one place, `version.txt`, and the engine reports it in its UCI name, `id name Elephant Gambit 1.0.0`. For Elephant the parts mean:

* **major**, an engine generation. A new architecture or a change to the UCI options, renamed, removed or behaving differently. 1.0 is the first complete, multi threaded UCI engine, an NNUE evaluation would be 2.0.
* **minor**, a finished cycle that made the engine stronger or added a feature, verified with SPRT.
* **patch**, fixes and cleanups without an intended strength change, verified not to lose strength.

Work happens in cycles focused on one area, on a cycle branch with one branch per change, each tested on its own. Builds in a cycle are pre-releases of the version it leads to, labelled with the cycle's name and a counter: `1.0.0-herd.1`, `1.0.0-herd.2`, ... A pre-release sorts before its release, the cycle ends with the release itself, `1.0.0`. Before 1.0 the cycles bumped the minor version and the patch for every change, `0.14.12-search`, released as `0.15.0`.

For the versions available, see the [tags on this repository](https://github.com/looak/elephant/tags). 

## Authors

* **Alexander Loodin Ek** - *Initial work* - [looak](https://github.com/looak)

[ci-link]:              https://github.com/looak/elephant/actions/workflows/ci.yml
[coverage-link]:        https://github.com/looak/elephant/actions/workflows/coverage.yml
[license-link]:         https://github.com/looak/elephant/blob/main/LICENSE
[release-link]:         https://github.com/looak/elephant/releases/latest
[commits-link]:         https://github.com/looak/elephant/commits/main

[lichess-link]:         https://lichess.org/@/elephantgambitengine
[uci-link]:             https://www.wbec-ridderkerk.nl/html/UCIProtocol.html

[head-link]:            https://github.com/looak/elephant/
[v1.0.0-link]:          https://github.com/looak/elephant/releases/tag/1.0.0
[v0.15.0-link]:         https://github.com/looak/elephant/releases/tag/0.15.0
[v0.13.3-link]:         https://github.com/looak/elephant/releases/tag/0.13.3
[v0.7.0-link]:          https://github.com/looak/elephant/releases/tag/0.7.0
[v0.6.5-link]:          https://github.com/looak/elephant/releases/tag/0.6.5
[v0.6.0-link]:          https://github.com/looak/elephant/releases/tag/0.6.1
[v0.4.0-link]:          https://github.com/looak/elephant/releases/tag/0.4.0
[v0.2.0-alpha.1-link]:  https://github.com/looak/elephant/releases/tag/0.2.0-alpha.1
[v0.1.0-alpha.1-link]:  https://github.com/looak/elephant/releases/tag/0.1.0-alpha.1


[ci-badge]:             https://img.shields.io/github/actions/workflow/status/looak/elephant/ci.yml?label=ci&logo=github&style=for-the-badge
[coverage-badge]:       https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Flooak%2Felephant%2Fbadges%2Fcoverage.json&logo=github&style=for-the-badge
[license-badge]:        https://img.shields.io/github/license/looak/elephant?style=flat-square
[release-badge]:        https://img.shields.io/github/v/release/looak/elephant?style=flat-square
[commits-badge]:        https://img.shields.io/github/commits-since/looak/elephant/latest?style=flat-square
[commit-activity-badge]:https://img.shields.io/github/commit-activity/w/looak/elephant?style=flat-square


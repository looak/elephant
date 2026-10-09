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
[![Build][build-badge]][build-link]
[![Test][test-badge]][test-link] </br>
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
|[v0.13.3][v0.13.3-link]|~18.3 million| N/A | ~1.5 million | testing |
|[v0.7.0][v0.7.0-link]|~19.5 million| N/A | ~1.65 million | ~1600 elo |
|[v0.6.5][v0.6.5-link]|~16.45 million| N/A | ~1.24 million | ~1500 elo |
|[v0.6.1][v0.6.0-link]|~20 million| N/A | ~1.97 million | N/A |
|v0.5.0|~11 million| N/A | ~1.65 million | ~1100 elo |
|[v0.4.0][v0.4.0-link]|~5 million|~110 million best case | ~600k | ~1350 elo |
|[v0.2.0-alpha.1][v0.2.0-alpha.1-link]| ~4 million | ~35 million best case | ~250k | ~1350 elo |

*moves p/s is perft from the start position, nodes p/s is search from the start position. v0.13.3 is measured on an AMD Ryzen 9 9900X, earlier versions on an AMD Ryzen 9 5950X, so the rows aren't directly comparable.*

v0.11.0, the last version before v0.13.3's evaluation and pruning work, reached the low 2000s in bullet, blitz and rapid. Since then PeSTO evaluation gained ~180 Elo and the new pruning ~120 Elo in self-play SPRT tests, see the [v0.13.3 release][v0.13.3-link].


## Features

* Engine:
    * bitboards with magic bitboard sliding attacks
    * staged legal move generation

* Search:
    * negamax alpha-beta with principal variation search
    * iterative deepening
    * transposition table
    * quiescence search with delta pruning
    * check extensions
    * null move pruning with verification
    * late move reductions
    * reverse futility pruning
    * futility pruning
    * move ordering: pv & transposition table move, MVV-LVA captures, killer moves, history heuristic

* Time management:
    * soft & hard limits, a new iteration only starts within the soft limit and the hard limit aborts the search
    * increment aware, never plans beyond what is on the clock
    * configurable move overhead for GUI & network latency

* Evaluation:
    * [PeSTO](https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function) material and piece-square tables, tapered between midgame and endgame by game phase
    * mop-up for converting won endgames

* API:
    * "user friendly" cli interface
    * UCI compatible
    * OpenBench compatible - https://github.com/AndyGrant/OpenBench

## Goals & todo

* evaluation: passed pawns, pawn structure and king safety, Texel tuning
* search: static exchange evaluation, aspiration windows, late move pruning
* time management: scale the soft limit by best move stability, score drops and nodes spent on the best move
* pondering
* multi threaded search (lazy SMP)
* a terminal UI
* ~~reach elo 2000~~

* github.io page?

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Prerequisites

This is a C++20 project and I'm not distributing any binaries, so to run the engine you need to compile it yourself.

Currently requiering a compiler which compiles [C++20](https://en.cppreference.com/w/cpp/20).

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
| `Threads` | 1 | 1–24 | search threads |
| `Hash` | 8 | 1–1024 | transposition table size in MB |
| `Move Overhead` | 10 | 0–5000 | milliseconds kept in reserve on every move for GUI & network latency, raise it when playing over the internet, e.g. 100 on lichess |

## Running the tests

Either run the output binary `ElephantTest` after build or browse to `.\build\` and execute `ctest`. `ElephantSuite` runs the slower perft and EPD (Win at Chess, Arasan) suites.

Changes to the search and evaluation are tested for strength with SPRT on [OpenBench](https://github.com/AndyGrant/OpenBench), which builds the engine with `ob_build/makefile`.

### Bench

OpenBench reads the bench, a node count from a fixed set of searches, from the newest commit's message as `bench <nodes> nodes`. The `hooks/prepare-commit-msg` hook adds it. When a commit changes the engine it builds with `ob_build/makefile` and g++, like OpenBench does, and runs the bench, through WSL on Windows. Other commits reuse the previous bench. Other compilers give different node counts, so the bench always comes from g++.

Install the hook once per clone, from the repository root:

```bash
cp hooks/prepare-commit-msg .git/hooks/ && chmod +x .git/hooks/prepare-commit-msg
```

`ELEPHANT_BENCH=force git commit ...` runs the bench even when no engine file changed, `ELEPHANT_BENCH=skip` leaves it out.

## Versioning

We use [SemVer](http://semver.org/) for versioning. The version is written in one place, `version.txt`.

Work happens in cycles focused on one area. A cycle bumps the minor version and labels it with the area, every change in it bumps the patch: `0.13.1-search`, `0.13.2-search`, ... A cycle ends with a release that drops the label, `0.13.3`. The engine reports the full version in its UCI name, `id name Elephant Gambit 0.13.3`.

For the versions available, see the [tags on this repository](https://github.com/looak/elephant/tags). 

## Authors

* **Alexander Loodin Ek** - *Initial work* - [looak](https://github.com/looak)

[build-link]:           https://github.com/looak/elephant/actions/workflows/build.yml
[test-link]:            https://github.com/looak/elephant/actions/workflows/test.yml
[license-link]:         https://github.com/looak/elephant/blob/main/LICENSE
[release-link]:         https://github.com/looak/elephant/releases/latest
[commits-link]:         https://github.com/looak/elephant/commits/main

[lichess-link]:         https://lichess.org/@/elephantgambitengine
[uci-link]:             https://www.wbec-ridderkerk.nl/html/UCIProtocol.html

[head-link]:            https://github.com/looak/elephant/
[v0.13.3-link]:         https://github.com/looak/elephant/releases/tag/0.13.3
[v0.7.0-link]:          https://github.com/looak/elephant/releases/tag/0.7.0
[v0.6.5-link]:          https://github.com/looak/elephant/releases/tag/0.6.5
[v0.6.0-link]:          https://github.com/looak/elephant/releases/tag/0.6.1
[v0.4.0-link]:          https://github.com/looak/elephant/releases/tag/0.4.0
[v0.2.0-alpha.1-link]:  https://github.com/looak/elephant/releases/tag/0.2.0-alpha.1
[v0.1.0-alpha.1-link]:  https://github.com/looak/elephant/releases/tag/0.1.0-alpha.1


[build-badge]:          https://img.shields.io/github/actions/workflow/status/looak/elephant/build.yml?logo=github&style=for-the-badge
[test-badge]:           https://img.shields.io/github/actions/workflow/status/looak/elephant/test.yml?label=test&logo=github&style=for-the-badge
[license-badge]:        https://img.shields.io/github/license/looak/elephant?style=flat-square
[release-badge]:        https://img.shields.io/github/v/release/looak/elephant?style=flat-square
[commits-badge]:        https://img.shields.io/github/commits-since/looak/elephant/latest?style=flat-square
[commit-activity-badge]:https://img.shields.io/github/commit-activity/w/looak/elephant?style=flat-square


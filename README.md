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

| Version | moves p/s<br>sngl core | moves p/s<br>mul core|nodes p/s<br>sngl core|[lichess.org]([lichess-link]) |
|:-------:|:---:|:---:|:---:|:---:|
|[future]([future-link])|~19.5 million| N/A | ~1.65 million | testing |
|[v0.7.0]([v0.7.0-link])|~19.5 million| N/A | ~1.65 million | ~1600 elo |
|[v0.6.5]([v0.6.5-link])|~16.45 million| N/A | ~1.24 million | ~1500 elo |
|[v0.6.1]([v0.6.0-link])|~20 million| N/A | ~1.97 million | N/A |
|v0.5.0|~11 million| N/A | ~1.65 million | ~1100 elo |
|[v0.4.0]([v0.4.0-link])|~5 million|~110 million best case | ~600k | ~1350 elo |
|[v0.2.0-alpha.1]([v0.2.0-alpha.1-link])| ~4 million | ~35 million best case | ~250k | ~1350 elo |

*all performance numbers are from running on my local machine, AMD Ryzen 9 5950x*


## Features

* Engine:
    * bitboards

* Search:
    * alpha beta neg max
    * quiescence search
    * transposition table
    * pv priority
    * killer heuristic
    * late move reduction
    * null move pruning

* Evaluation:
    * material
    * position tables
    * tapered position evaluation
    * king safety
    * passed pawn
    * mop-up

* API:
    * "user friendly" cli interface
    * UCI compatible
    * OpenBench compatible - https://github.com/AndyGrant/OpenBench

## Goals & todo

* multi threaded search
* reach elo 2000

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

Interfacing with elephant can nativly be done through ElephantCLI. As of [v0.4.0]([v0.4.0-link]) supports [UCI protocol]([uci-link]) and you can interface it with your Chess GUI of choice. Personally, I have been using [Arena](http://www.playwitharena.de/) & [CuteChess](https://cutechess.com/). Every so often I'll host the engine locally and one can play against it on [lichess.org]([lichess-link]).

## Running the tests

Either run the output binary `ElephantTest` after build or browse to `.\build\` and execute `ctest`.


## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/looak/elephant/tags). 

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
[future-link]           https://github.com/looak/elephant/tree/future
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


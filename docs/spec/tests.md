# Elephant test binaries

Repo: [looak/elephant](https://github.com/looak/elephant), `test/` · C++20, GoogleTest · true at the commit it is read
at

## Purpose

Two GoogleTest binaries linked against the engine library, so the engine is tested from the inside: move generation,
position bookkeeping, search behaviour and the UCI layer, without a process boundary. Correctness only; strength is
the test pipeline's ([test-pipeline.md](test-pipeline.md)), OpenBench's and uwanja's. phant-indexer, nyayo's game
reader, is tested here too, and checked against python-chess by a script in the pipeline.

| Binary | ctest label | Holds | Takes | Runs in | Why there |
|:--|:--|:--|:--|:--|:--|
| `ElephantTest` | `unit` | ~530 unit tests (~580 with the indexer's), shallow perft of every perft position, search expectations to depth 12 | ~6 s | CI `unit` on every push & PR to main; smoke step 1; the edit loop | function-level correctness, fast enough to run after every change |
| `ElephantSuite` | `suite` | `PerftFixture.ReferencePositions` (~0.8 billion nodes), `PerftFixture.DeepPositions` (~22 billion), `SearchDepthFixture.*` (moves found only deep in a search) | ~15 s on 24 threads, perft on every core | CI `perft` (perft only); smoke step 3 (all of it) | perft is the authority on move generation and deterministic, so CI gates on it; the deep searches move with the evaluation, so only smoke runs them |

Both come from `elephant_add_test_executable()` in `test/CMakeLists.txt`, which registers every test with ctest under
its label (`ctest -L unit`). `ELEPHANT_BUILD_TEST_SUITE=OFF` leaves `ElephantSuite` out. `ElephantTest` includes
the indexer's tests only when `ELEPHANT_BUILD_INDEXER` is on (off by default, phant-indexer needs it) and exceptions
are enabled; the engine's tests are the same either way. `ELEPHANT_COVERAGE` instruments `ElephantTest` and what it
tests for a coverage report, below.

## Scope

**Does:** prove the engine correct at the function level: legal moves, make/unmake round trips, hashing, FEN, SAN &
PGN, evaluation symmetry, SEE, the transposition table, time management, UCI command handling, and a few search
expectations (mate in N, the zugzwang positions); and prove phant-indexer's entries, input and position hashes.

**Doesn't:** measure strength or speed, or keep history, coverage included. Results go to the console and ctest
only; history of EPD positions is the test pipeline's and nyayo's job ([test-pipeline.md](test-pipeline.md)).

## Current

### Perft positions

`test/inc/perft_cases.hpp` lists every perft position with its node count at each depth: the six standard positions
from the chessprogramming wiki (every depth published, up to 8 billion nodes) and 15 special cases (en passant
legality, castling, promotions, stalemate) whose deepest count is published and shallower ones are the engine's own.
Each binary runs a position at the deepest depth within its budget: `ElephantTest`'s `PerftShallow` up to 1 million
nodes, `ElephantSuite`'s reference positions up to 250 million, the deep positions at their deepest listed depth.

### ElephantTest

`test/src/<area>/*_test.cpp`, areas `bitboard`, `io`, `move`, `position`, `search`, `system`, and `indexer` (with
`ELEPHANT_BUILD_INDEXER`). Shared fixtures and data in `test/inc`: `chess_positions.hpp`, `perft_cases.hpp`, `search_cases.hpp` (expected moves, including
`s_nullMovePruning`, the zugzwang cases that guard pruning), `move_gen_ab.hpp`.

- **Move generation** is tested through `move_gen_ab.hpp`, a harness that can run the same tests over more than one
  generator. Only the staged "tusk" generator is left since the legacy one was removed in 1.0; the harness stays for
  the next one. `PerftShallow` runs every perft position in milliseconds, and checks that `divide`'s per move counts
  add up to the perft count.
- **The position** (`position/position_test.cpp`), a fixture per area: the basics (empty, one of each piece, the
  starting position, copies & equality), edit access (validated & unvalidated placing and clearing, the square
  operator, the material editor, `restoreState`, readers), board iteration, hashing (the kept hash equals one
  computed from scratch after every kind of edit, and through make & unmake of castling, en passant and promotions),
  the material mask, the squares the material threatens, castling & en passant state, and FEN round trips of
  positions built piece by piece. Each test that sets up a position draws the board above it, white uppercase, black
  lowercase, `x` on the squares the test is about.
- **Evaluation symmetry**: `evaluation_test.cpp` flips each position (ranks mirrored, colours swapped) and expects the
  same score from the other side.
- **Search expectations** (`search_test.cpp`) search to a fixed depth, 12 by default, and expect a move.
  `NullMovePruning_ExpectedMove` holds the zugzwang positions found at depth 8 & 9.
- **SAN & PGN** (`io/san_parser_test.cpp`, `io/pgn_parser_test.cpp`): the parsers that read games are tested against
  what OpenBench, fastchess and the PGN standard write, with python-chess's reading (`chess.pgn.read_game`) as the
  reference for the expected positions. SAN covers piece, file, rank and square disambiguation, captures with and
  without `x`, promotions with and without `=`, castling with letters and zeros, ignored `+`/`#`, pinned pieces and
  an ambiguous or illegal move throwing. The tokenizer and game reader cover move numbers, `{}` and `;` comments,
  NAGs, `!?` suffixes, nested variations, `%` lines, null moves (`--`, `Z0`, `0000`, `@@@@`), moves after the
  result, blank lines ending a game (not in a comment), tags read as python-chess reads them (values as written, a
  line that isn't a tag skipped), a `FEN` tag with or without `SetUp`, and a `Variant` that isn't standard chess
  throwing. `PgnFixtures.EveryGameIsPlayed` plays every game in `test/pgn` to its `PlyCount` where it has one, the
  directory compiled in as `ELEPHANT_TEST_PGN_DIR`.
- **PGN fixtures** (`test/pgn`): `openbench_workload.pgn` (34 games as an OpenBench worker uploads them, from openbench_eg's
  `UnitTests/example3.pgn`), `openbench_unstripped_1.pgn` and `_2.pgn` (cutechess-ob's games before OpenBench strips
  them), `fastchess_gauntlet.pgn` (8 games as uwanja's gauntlets write them), `edge_cases.pgn` and
  `python_chess_quirks.pgn` (what the standard allows and `read_game` does that engines don't write). The phant-indexer
  check against python-chess reads them too.
- **Assertions builds** (`ASSERTIONS_ENABLED`) check invariants inside the engine too, e.g. that the incremental
  hash equals a recomputed one at every alphaBeta and quiescence node.

### phant-indexer

`test/src/indexer/` (only with `ELEPHANT_BUILD_INDEXER`):

- `game_index_test.cpp` pins the cards: the JSON line byte for byte (format 2), the facts on games picked for them
  (promotions with a capture, castling plies, en passant, checks, the fewest pieces, material at fixed plies, every
  way a game ends by the rules), an illegal move's ply, a `FEN` that doesn't read, a `Variant` that isn't standard
  chess, the result rules, repeated tags, null moves, and the `start` and `opening` hashes.
- The same file pins the `--positions` record (its 14 bytes, the move's packed bits for a quiet move, a capture, a
  promotion with a capture, castling both ways, an en passant capture and a null move, and none at the last position).
- `input_test.cpp` covers detecting plain, bz2, gzip and zstd by the first bytes, concatenated streams, zstd frames with
  and without their content size and with a skippable frame between, and cut streams being an error.
- `draw_test.cpp` covers `--draw`: positions with a capture to make left out under `--quiet`, none in check or
  checkmated, `--min-ply` counted from where the game starts whatever its FEN's move number, a file's `FILE:P` and a
  `DrawMinPly` tag overriding it (and `FILE:P`'s parsing), the statistics JSON byte for byte for a plain and a merged run,
  `--merge` averaging the results of the games that drew a position and repeating, and the per-game cap, a sample that repeats with its seed and changes with another, a
  position written once in a run, nothing from a game that didn't read, has no result or has a null move, and a game
  from a FEN.
- `polyglot_test.cpp` checks the Polyglot hash against all nine test vectors Polyglot's book format documents, from
  moves and from FEN, and the rules python-chess follows where they matter: an en passant file hashed only with a
  pawn to take, a castling right counted only with the king and rook at home.

Every hash and every fact is also compared with python-chess over many games by `test/indexer_smoke.py`, which is
part of the pipeline: [test-pipeline.md](test-pipeline.md#phant-indexer-against-python-chess).

### ElephantSuite

`test/suite/`:

- `perft_test.cpp`, `PerftFixture.ReferencePositions` & `PerftFixture.DeepPositions`: perft node counts with the
  tusk generator, the authority on move generation. Each position is split two plies deep and the subtrees are
  shared out between threads, a GameContext per thread; `EG_PERFT_THREADS=N` sets the thread count (default every
  core). The last ply is counted in bulk, the generator's popcount of each piece's legal targets, never made. ~5
  billion nodes per second on 24 threads, ~530 million on one.
- `search_depth_test.cpp`, `SearchDepthFixture.NullMovePruning_DeepLine`: d4e3, a null move pruning line found only at
  depth 22 (searched to 23, ~5 s); its comment records the depth history. Raise the depth when new pruning or
  evaluation pushes it out of reach.

### Coverage

`ELEPHANT_COVERAGE=ON` (`cmake/options.cmake`, off by default) links `coverage_flags` (`cmake/coverage.cmake`) into
`engine`, `cli`, `PhantIndexerLib` and `ElephantTest`, never googletest, the compression libraries or `third_party`:
`--coverage` with gcc, `-fprofile-instr-generate -fcoverage-mapping` with clang and clang-cl (clang-cl's link gets
the profile runtime's directory), both with atomic counters (`-fprofile-update=atomic`), the search's threads race on
them otherwise. It turns link time optimization off; the scripts build Debug so lines map one to one.
`ElephantSuite` is never measured, too slow instrumented. `cli` is instrumented but `ElephantTest` doesn't link it,
so it isn't in the report.

- `test/coverage.ps1 [build dir]` (Windows, clang-cl, loads Visual Studio's developer environment when needed):
  configures `build-cov` (ignored by git) with coverage, Debug, `ELEPHANT_BUILD_INDEXER=ON`, the suite & tuner off,
  builds `ElephantTest`, runs `ctest -L unit` with `LLVM_PROFILE_FILE` set (a pool of profiles the test processes
  merge into), merges them with `llvm-profdata` and writes `llvm-cov show`'s HTML to `build-cov/coverage/index.html`,
  then prints `llvm-cov report`. Exit code: the tests'.
- `test/coverage.sh [build dir]` (Linux, gcc, needs gcovr): the same with gcovr, throw branches left out, HTML to
  `<build dir>/coverage/index.html` and the text summary to the console.

Both leave out sources under `test/`, `third_party/` and `_deps/`. Nothing is recorded or uploaded, and no number
gates anything.

### Where they run

| Where | How | Kept |
|:--|:--|:--|
| the edit loop | `ElephantTest`, or `ElephantSuite --gtest_filter=PerftFixture.*` after a move generation change | console |
| `test/smoke.sh`, locally before a merge | `ElephantTest --gtest_brief=1`, then all of `ElephantSuite`, then phant-indexer against python-chess | pass/fail in the smoke summary, logs in `logs/` |
| CI `ci.yml` job `unit`, pushes & PRs to main, both OSes | `ctest -L unit`, built with assertions & exceptions and `ELEPHANT_BUILD_INDEXER=ON`, suite off, warnings as errors | thrown away |
| CI `ci.yml` job `perft`, pushes & PRs to main, both OSes | `ctest -L suite -R "^PerftFixture\."`, warnings as errors | thrown away |
| CI `coverage.yml`, run by hand on any branch, ubuntu-24.04 | `ctest -L unit` built with gcc and `ELEPHANT_COVERAGE=ON` (Debug), then gcovr's text summary | the job's summary page, thrown away; on `main` the line coverage replaces `coverage.json` on the `badges` branch, the README's badge |
| `test/coverage.ps1` / `test/coverage.sh`, locally by hand | `ctest -L unit` instrumented | HTML report in `build-cov/coverage/` |

## Interfaces

| Direction | What |
|:--|:--|
| in | the engine library (`engine` target), `res/` for test data |
| out | gtest console output & exit code, ctest labels `unit` & `suite` |
| out | coverage: an HTML report in `build-cov/coverage/`, a text summary on CI's summary page, `coverage.json` (shields.io endpoint format: `schemaVersion`, `label`, `message` the line percent, `color`) on the `badges` branch for the README |


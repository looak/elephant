# Search refactor: where it stands

Pick-up notes for the refactor behind #47. Delete this file when the work is done.

## The goal

Clear ownership and clean APIs around search: each piece of state lives where its lifetime says it should, search never
prints or reads the live game, and UCI only translates text. Every step so far keeps the bench unchanged, except
where noted.

## Where the branches are

```
main
  search-time
    refactor-thread-context    PR #51 into search-time, open
      search-request           pushed, no PR yet
        refactor-monad         this branch
```

`search-request` should get a PR into `refactor-thread-context`. CI only runs by itself on pull requests into `main`, on these
branches start it with `gh workflow run ci.yml --ref <branch>`.

## Done

On `refactor-thread-context`, merged:

- `SearchContext`, state that outlives a `go`, per thread. Histories & pawn hashes carry over between searches.
- The TT reaches search through the thread context, the static pointer is gone. The scout counters are gone.
- #53: a `stop` right after `go` is kept, a valid `setoption` ends a running search first. Both races were reproduced by
  taking the fixes out: the stop test fails on its first attempt, the Hash test segfaults.
- #54: search files sorted into `core/`, `state/` & `perft/`, `ThreadContext`, `SearchListener`, the indexer build fix.

On `search-request`:

| Commit | Change |
|---|---|
| `b6a7bf7` | `SearchRequest`: a search gets its own copy of the root, repetition hashes & limits. `SearchParameters` is `SearchLimits`, `SearchInputs` is gone, the thread count belongs to `SearchContext`, `MoveHistory` splits into shared game hashes & a per-thread line |
| `639167a` | `NodeKind` & `RootMoveSource`: search's shared loop no longer knows about `searchmoves`, a plain `go` has no check for it |
| `4470428` | Tests: moves with flags in `searchmoves`, the table's best move left out |
| `f51abdc` | `GameState` leaves `ThreadContext` |
| `2ece32b` | `plyCount` & `readPly` renamed `halfmoveClock` & `readHalfmoveClock` |

Checked on the tip of `search-request`, g++ 16 in WSL and MSVC 19.52: full build with warnings as errors, 598 of 598 unit
tests, bench 467534. The OpenBench makefile build has no warnings. `ElephantSuite` passes on g++.

On `refactor-monad`:

- `Chessboard::makeMove`, `unmakeMove` & `makeNullMove`, one place plays a move and keeps the side to move & clocks in
  step. `io::fen_parser::read`, one place reads a FEN without throwing. `game_index.cpp`'s FEN tag keeps `deserialize`,
  its error text carries the parser's message and that's in the indexer's output.
- `QuiescenceProbe`, the setup the tuner and indexer each repeated. `Search::quiescenceLine` is private to it. `Search` is
  going away, so the probe takes it along in step 8.
- The tuner's `EntryBuilder` and `Drawer::draw`'s steps split out.

- `Monad`: owns the TT, which left `GameContext`, and a `SearchContext`, and creates a `Search` per `go`. `go` returns
  immediately, `wait()` gives the result, the listener hears `onBestMove` or `onSearchFailed`. An infinite search that
  ends on its own keeps its answer until `stop`. A `TimeManager` per `go`, so a stop can't be lost or carry over.
  Its destructor stops and joins, `go infinite` then `normal` no longer hangs. Until step 6 the CLI's `search` has its
  own table per command, it had the `GameContext`'s.

Bench doesn't cover the tuner or indexer. These were checked by comparing their output with `search-request`'s on
`test/pgn` and a dataset drawn from it: entries, positions, draws, stats, tuned tables.

## How it fits together

Three layers, by lifetime. Nothing clears a history: a `Game` starts at a position and is replaced whole.

| Construct | What it is | Lives for | Owner |
|---|---|---|---|
| `Chessboard` | `Position`, side to move, clocks. Copyable | as long as its holder wants | anyone: `Game`, indexer, tuner |
| `Game` | a start position and the moves played | one start position, until replaced | the interface (UCI, CLI, a test) |
| `Monad` | the TT, the workers' tables, options, the running search | the process | the interface |
| `SearchRequest` | the question: root, game hashes, root moves, limits | one `go`, immutable | the search |
| `TimeManager` | the limits and the stop signal | one `go` | `Monad` |
| `ThreadContext` | working position, accumulator, current line, killers, counters | one `go`, one thread | the search thread |

| Event | `Game` | TT | History tables, pawn hash | Killers |
|---|---|---|---|---|
| `position` | replaced | kept | kept | n/a |
| `ucinewgame` | replaced | zeroed | zeroed | n/a |
| `go` | copied into the request | kept, age incremented | kept | fresh |

Decided:

- **`Monad`** is the name, after Leibniz: its own memory, nothing shared with another one, no colour of its own. The class
  comment says so.
- **The interface owns the `Game`,** not `Monad`. `SearchRequest` is already a complete copy, so self-play (#49) is one
  `Game` and two `Monad`s. UCI and the CLI hold both, a small `Session` in the CLI covers that.
- **`Monad` owns the TT,** one per `Monad`. Its workers borrow it for a `go`.
- **`Game` replaces `GameContext`,** which is a board and an undo stack once the TT has left.
- **`Chessboard` stays.** The indexer and tuner use it without a history.
- **`SearchHandle`** is left out: one `Monad` runs one search at a time, and #53 closed the race it was meant to prevent.

## What's left, in order

4. **`Game`.** `GameContext` shrinks and is renamed. `age`, `IsRepetition`, `editChessboard` & `editGameHistory` go,
   `MoveHistory` becomes `RepetitionHistory`, heuristics' `history` becomes `butterfly`. `PerftSearch` takes a `Game` by value,
   `pgn_parser` returns one.
5. **`position` builds a new `Game`.** Fixes `position fen` never clearing the history (`uci.cpp`): every `position fen ...
   moves ...` adds the moves again, and with a non-zero FEN clock and no capture or pawn move since, a position that occurred
   twice can count as threefold. Changes play, bench can't see it: its own commit, a test and an SPRT.
6. **`Session`, one bench loop.** UCI and the CLI's commands hold a `Game` and a `Monad`. Bench builds its own, so it leaves
   the session alone, and shares the loop but not the output, which `docs/spec/engine.md` specifies per binary. The CLI's
   `search` stops building its own `SearchContext`, so histories carry over in normal mode. `readGameContext` &
   `editGameContext` move over gradually, 40 uses in `uci_test.cpp`.
7. **`UCI` moves to `src/cli`,** with `UciSearchListener`. The tests then link the CLI library, `ob_build/makefile` lists CLI
   sources by name, and the bench hook doesn't watch `src/cli/inc`. Rename the CLI's `namespace elephant`.
8. **The per-ply struct, then `Worker`.** Gather `halfmoveClock[]`, `nullMoveAt[]`, the continuation stack and the killers
   into one struct per ply, #47's. Then `Search` splits: starting & joining threads, the shared stop & node count and the
   listener go to `Monad`, `alphaBeta` & friends to `Worker`, and `SearchContext`'s parallel vectors become
   `std::vector<Worker>`. A `Worker` is a thread's tables, not a thread: threads still start per `go`. Layout changes in the
   hot path can move nps with bench unchanged, so check nps as well. Compare `Worker` with the other engines first.

Update `docs/spec/engine.md` and `docs/architecture.md` in the same pull request as each step they describe.

## Parked

- **`QuiescencePolicy::maxDepth`:** mutable static that nothing writes, could be `constexpr`. Left on purpose.
- **A real root move list:** a list of all legal moves at the root with data per move (score, nodes, PV), generated once
  and re-sorted between iterations. Today's `SearchRequest::rootMoves` is only the allowed moves, and the root generates
  moves on every visit. It would suit best move stability in the 1.5 cycle, and `NodeKind` gives it a home. It changes
  ordering for every search, so it changes bench and needs an SPRT. `allowedRootMoves` would be a more accurate name for
  the field until then.
- **Shorter mates on `go infinite`:** iterative deepening breaks out at the first mate (`search_impl.cpp`), so the
  search stops looking while it holds for `stop`. MultiPV isn't needed, a shorter mate scores higher on the normal
  line: limit the break to searches that aren't infinite. Only `go infinite` & `go ponder` change, bench doesn't.
- **Pluggable move ordering:** scoring sits inside the generator, `move_generator.cpp`'s `scoreMove`. A separate ordering
  type that scores and learns, with the generator only knowing the rules, was discussed but not started.
- **SPSA:** a params registry as the one place a search constant is written, generating the release `constexpr`s, the
  tuning build's UCI options and OpenBench's input. Not started, 1.5 plans an SPSA pass after the search changes.
- **#50:** `ElephantSuite`'s only search test is disabled, so the suite is perft only right now.
- **Sharing one `Session` across modes:** with `AppContext` owning it, switching from normal to `uci` could keep the
  position. A behaviour change, not part of the first step.

## Things to remember

- **Other engines:** when a design choice draws on precedent, look at Ethereal, Triumviratus & Pawnocchio first and ask
  why they differ. Stockfish is one reference and a check for correctness, not the model. `NodeKind`, `Worker` &
  the root move list were described from memory of Stockfish, and haven't been compared with the others. `Worker` is
  worth checking before it's built.
- **Commits:** renames & cleanups are their own commits with the bench line unchanged, see `CLAUDE.md`.
- **Bench can't see everything:** its positions have no game history, so it doesn't reach the game side of
  `isRepetition`, and it doesn't use `searchmoves`. Those rely on unit tests.
- **Check failing builds carefully:** a failed `cmake --build` leaves the old binaries in place, so tests and bench can
  still pass after it. Read the build's exit code first.
- **`ThreadContext` lifetime:** it holds references to a `SearchRequest` & a clock, so the tuner and indexer each keep
  their own, with a 1 MB TT quiescence never touches.

## Building

The prepare-commit-msg & post-commit hooks in `hooks/` aren't installed by default on a new clone, and the bench they add
is built with g++ through `ob_build/makefile`.

```bash
cd ob_build && make -j8 EXE=build/elephant_ob && ./build/elephant_ob bench | tail -1
```

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DASSERTIONS_ENABLED=ON -DEXCEPTIONS_ENABLED=ON -DELEPHANT_BUILD_INDEXER=ON -DELEPHANT_BUILD_TEST_SUITE=ON -DCMAKE_COMPILE_WARNING_AS_ERROR=ON
```

```bash
cmake --build build --parallel 8 && ctest --test-dir build -C Release -L unit
```

Binaries land in `build/Release/bin`. Bench should print `467534 nodes` on both g++ and MSVC.

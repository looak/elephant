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
      search-request           this branch, pushed, no PR yet
```

`search-request` should get a PR into `refactor-thread-context`. CI only runs on pull requests into `main`, so nothing
on these branches is checked unless it's built by hand.

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

## What's left, in order

### 1. `Elephant` (called `Engine` in earlier discussion)

A class of its own for the engine, separate from the UCI protocol. It takes the TT, `SearchContext`, clock, options &
current game away from `UCI`, and replaces `GameContext` as what the CLI's commands hold. No change in play, bench stays.

```cpp
class Elephant {
public:
    void setThreads(u16 count);
    void setHash(size_t megabytes);
    void setMoveOverhead(u32 milliseconds);

    void newGame();
    bool setPosition(std::string_view fen);
    void makeMove(PackedMove move);
    const GameContext& game() const;     // read only, for print, evaluate, perft

    void go(SearchLimits limits, SearchListener& listener);  // returns immediately
    void stop();
    void wait();
};
```

What moves or changes:

- `UCI` keeps parsing text & formatting output, and holds an `Elephant`. Its rules ("a valid setoption ends the search
  first", "reset the stop state before starting") move into `Elephant`, so every caller gets them.
- `NormalModeProcessor` & the commands hold an `Elephant` instead of a `GameContext*`, see `command_api.hpp`. `search`
  stops building its own `SearchContext`, so histories carry over in normal mode as they do in UCI.
- The three benches (`UCI::Bench`, `bench_command.hpp`, `openbench.cpp`) become one loop. `openbench.cpp` has its own FEN
  list, check it against `bench_positions.hpp`. `BenchScope` goes away: it's `newGame` & `setPosition`.
- `UCI` moves out of the engine library into `src/cli`, the engine no longer depends on a protocol. Add it to
  `docs/architecture.md`.

Things to check while building it:

- `PerftSearch` takes a non-const `GameContext&`. If it edits it during its walk, a read only `game()` means it needs its
  own copy or a position.
- `readGameContext` & `editGameContext` on `UCI` are used by tests. Move them over gradually.
- `namespace elephant` in the CLI's `static_initializer` compiles alongside the class but reads badly, rename it.
- Keep each `Elephant` able to live next to another in one process, #49 (in-engine self-play) needs that.

**Open: `SearchHandle`.** The sketch has `stop()` & `wait()` on `Elephant`. A handle object was proposed earlier, but one
`Elephant` runs one search at a time and #53 closed the race it was meant to prevent with a one-line move. Leaning
towards leaving it out unless it earns its place. Also, `SearchListener` has no `onBestMove` yet, that comes with `go`
returning immediately.

### 2. `Game`

Once the TT has moved into `Elephant`, `GameContext` is only a `Chessboard` (`Position` + `GameState`) and the history.
Fold them into one type, a separate step from the first. This does the `TODO: Probably just remove chessboard
entirely` in `chessboard.hpp`. The FEN parser, indexer & CLI commands use `editState()`, so they need a clear place to go
first, which is why this waits for `Elephant`.

### 3. `Worker`

`alphaBeta` & friends move off `Search`, which only ends up starting & joining threads. A `Worker` runs a `ThreadContext`.
`threadId` & `ThreadHeuristics` keep their names for now.

Also #47's per-ply struct, which is the thread stack rewrite: gather `halfmoveClock[]`, `nullMoveAt[]` & the continuation
stack into one struct per ply, possibly with the static eval & killers. Debug logging leaves `thread_context.hpp` for a
file of its own, about 85 lines. Decide `SearchContext`'s parallel vectors at the same time, one vector of
`{ThreadHeuristics, PawnHashTable}` is the persistent half of a `Worker`.

## Parked

- **`QuiescencePolicy::maxDepth`:** mutable static that nothing writes, could be `constexpr`. Left on purpose.
- **A real root move list:** a list of all legal moves at the root with data per move (score, nodes, PV), generated once
  and re-sorted between iterations. Today's `SearchRequest::rootMoves` is only the allowed moves, and the root generates
  moves on every visit. It would suit best move stability in the 1.5 cycle, and `NodeKind` gives it a home. It changes
  ordering for every search, so it changes bench and needs an SPRT. `allowedRootMoves` would be a more accurate name for
  the field until then.
- **A todo that was lost:** `// todo: remove root moves, probably solved by replacing the mov generator or something` went
  with `SearchInputs`. Search no longer checks root moves itself, but the list & `RootMoveSource` remain. Put it back
  somewhere if it still applies.
- **Pluggable move ordering:** scoring sits inside the generator, `move_generator.cpp`'s `scoreMove`. A separate ordering
  type that scores and learns, with the generator only knowing the rules, was discussed but not started.
- **SPSA:** a params registry as the one place a search constant is written, generating the release `constexpr`s, the
  tuning build's UCI options and OpenBench's input. Not started, 1.5 plans an SPSA pass after the search changes.
- **#50:** `ElephantSuite`'s only search test is disabled, so the suite is perft only right now.
- **Sharing one `Elephant` across modes:** with `AppContext` owning it, switching from normal to `uci` could keep the
  position. A behaviour change, not part of the first step.
- **A real strength check:** history carrying over between searches (`e7f11c7` on `refactor-thread-context`) changes
  play. Its commit message mentions no SPRT, run one if that hasn't happened.

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

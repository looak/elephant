# Search speed profile

Where a search node's time went on 2026-11-02, at commit 2db77ee on `refactor-worker`, and what could be done about it.
Nothing here has been implemented. The savings under Suggested changes are estimates, none of the changes has been tried.

The question was whether 10M nps is within reach, the search running at about 3.3M. In short: not with this eval. 10M nps
is 489 cycles per node, the search takes 1421 and eval alone 332. About 4M looks reachable without touching the eval's
design.

## Findings

Middlegame position from `test/perf.sh` at depth 18, `Hash` 64, one thread pinned to one core of a Ryzen 9 9900X under
WSL. One position and one run per counter: other phases differ, the endgame workload runs at about 4.9M nps.

| Where | Share of cycles | Cycles per node |
|---|---|---|
| Move generation and ordering | 31.7% | 451 |
| Eval | 23.4% | 332 |
| Search functions (`alphaBeta`, `searchMoves`, `quiescence`, small helpers) | 11.9% | 168 |
| Make/unmake | 8.4% | 120 |
| `CheckInfo` | 7.8% | 111 |
| TT | 7.4% | 105 |
| SEE | 3.6% | 51 |
| Slider lookups and `pieceAt`, where not inlined | 2.8% | 39 |
| libc (not identified, possibly `memcpy`) and other | 2.6% | 37 |

Per node: 3448 instructions at 2.43 per cycle, 10.9 branch misses, 8.6 L1 data misses, 8.4 L1 instruction misses. The
front end is idle for 21% of cycles.

What the search did over the 24,791,979 nodes:

| Count | What |
|---|---|
| 16,648,445 | `alphaBeta` entered |
| 922,030 | TT cutoffs |
| 6,618,169 | dropped into quiescence |
| 9,108,242 | reached `moves.peek()` |
| 689 | of those, no moves: mate or stalemate |
| 4,418,082 | RFP cutoffs, after the peek |
| 357,019 | null move cutoffs, after the peek |
| 4,332,452 | reached the move loop |
| 3,088,626 | beta cutoffs in the loop, 2,755,738 of them (89%) on the first move |
| 14,829,385 | quiescence entered, 6,524,008 stand pat cutoffs |
| 24,859,657 | `CheckInfo` built |
| 22,969,393 | evaluations |
| 16,626,488 | captures batches generated, 8,162,091 of them by the peek |
| 1,723,580 | quiets batches generated, 148,033 of them by the peek |
| 88,586,176 | moves generated and scored |
| 6,036,057 | `isLegal` calls |

Why perft's speed says little here: perft never pays for scoring, sorting or the check square lookups, and search
generates at nodes that never use the moves.

The refactor neither gained nor could have gained much: it moved state that is set up once per `go`, not per node. The
branch misses are mostly in move generation and ordering, which it didn't touch.

### Release history

The same position, 3 rounds, best nps. Node counts differ between versions except 1.3.0 and 1.3.2, so the small steps are
approximate: a different tree is a different mix of nodes.

| Version | Nodes | nps | What changed |
|---|---|---|---|
| 1.0.0 | 6,632,371 | 4.50M | |
| 1.1.0 | 7,771,149 | 3.89M | mobility |
| 1.2.1 | 26,688,740 | 3.75M | pawns |
| 1.3.0 | 8,554,469 | 3.54M | king safety |
| 1.3.2 | 8,554,469 | 3.55M | danger map, no slowdown |
| 1.4.0 | 14,426,571 | 3.52M | piece terms |
| 2db77ee | 24,791,979 | 3.32M | search changes and refactor |

1.0.0 to 1.4.0 lost 22% nps and gained about 260 Elo by the gauntlet estimates, a good trade. Even 1.0.0 only reached 4.5M.

### Ruled out

- **PGO**: g++ 13 with `-flto`, total +0.47% trained on positions outside the workloads, -0.97% trained on the workloads
  themselves. The profile did apply: no missing profile or mismatch warnings, a binary 16% smaller. Only the endgame gained,
  about 5%. Not tried with another compiler.
- **The danger map as a regression**: 1.3.0 and 1.3.2 run at the same nps on identical nodes. It's still worth deferring,
  see below, but it isn't what made search slower.
- **LTO and TT prefetch**: both already in.

Templates don't look like the main problem, since a smaller binary from PGO didn't help speed. Their instruction cache
cost isn't ruled out, code size per instantiation wasn't measured.

## Suggested changes

| Change | Saves, estimated | Bench |
|---|---|---|
| Move `moves.peek()` after RFP and null move | ~6.5% | may change |
| Compute the danger map only when king moves need it | up to ~5% | same |
| Sort quiets lazily | ~3% | same if ties keep their order |
| `pieceAt` out of line, the `RepetitionHistory` vector | 1-3% | same |

Together 15-20%, about 4M nps. Start with the peek, it's the largest. A change that keeps the bench only needs
`test/perf.sh` to show the speed, one that changes it needs a non-regression SPRT as well.

**`moves.peek()`** in `search_alphabeta.cpp`. The mate check runs before RFP and null move and costs 12.4% of the search:
52% of those nodes are pruned straight afterwards, and only 689 of 9.1M peeks found no moves. The loop would have to detect
mate and stalemate itself, telling "no legal moves" from "all moves skipped" by futility or late move pruning. A stalemate
that now gets pruned can change the bench.

**Danger map** in `check_info.hpp`. Built at every node, 24.9M times, while only the 1.7M quiets batches always need it, and
captures batches only when the king touches an enemy piece. Slider checks already come from the sniper loop, pawn and
knight checks are two lookups, so check detection doesn't need it. `CheckInfo` also takes 39% of the cache miss samples.

**Sorting** in `MoveGenerator::generateScored`. 5.7% of cycles and 14% of branch misses, yet 89% of cutoffs come on the
first move, so most of that order is never used. Picking the best on demand gives the same order if ties stay in generation
order. `MoveGenResult::searched()` relies on the buffer's order, so that needs care.

**Small items.** `MaterialPositionMask::pieceAt` isn't inlined and the search calls it for a piece `makeMove` already knows.
`RepetitionHistory::push` shows 2.8% on `vector::push_back`, some of which may be skid from the prefetch just before it. A
fixed array by ply removes the capacity check.

Going well past 4M means making eval cheaper, for example by sharing attack maps between mobility, king safety and
`CheckInfo`. A bigger design change, but it's where the largest remaining cost is.

## How it was measured

All builds were throwaway, outside the repo, with the `ob_build/makefile` flags. `-g` was added for symbols and the bench
stayed at 467534. WSL's `perf` reads the hardware counters on this machine but is built without libelf or libdw, so it can't
name anything.

```sh
# the build to profile, from ob_build. For stacks add -fno-omit-frame-pointer -fno-optimize-sibling-calls
make -j12 CXX="g++ -g" BUILD_DIR=$HOME/prof/obj EXE=$HOME/prof/eg-uci-prof

# counters, each group a run of its own. The search is driven over a fifo as in test/perf.sh and quits at bestmove
perf stat -e cycles:u,instructions:u,branches:u,branch-misses:u -- taskset -c 4 ./eg-uci-prof
perf stat -e cycles:u,stalled-cycles-frontend:u -- taskset -c 4 ./eg-uci-prof
perf stat -e L1-dcache-loads:u,L1-dcache-load-misses:u,L1-icache-load-misses:u,cache-misses:u -- taskset -c 4 ./eg-uci-prof

# samples, also with -e branch-misses:u, cache-misses:u and L1-icache-load-misses:u for where those land
perf record -F 4000 -e cycles:u -o flat.data -- taskset -c 4 ./eg-uci-prof
# the frame pointer build, for inclusive time
perf record -F 2000 -e cycles:u --call-graph fp -o fp.data -- taskset -c 4 ./eg-uci-prof-fp
```

- **Names**: `perf script -F ip,dso` gives each sample's address and `perf script --show-mmap-events` where the binary was
  mapped, so address - map start + map offset is an address in the file. `addr2line -a -f -i -C -e <binary>` gives its
  chain of inlined functions and source line. Innermost function is self time, every function in the chain inclusive. LTO
  inlines most of the search into a few functions, so this says more than symbols alone would.
- **Inclusive time and callers**: `perf script -F ip` on the frame pointer recording gives the stacks, `nm -C -n` names the
  frames. Counting a function once per sample keeps recursion from inflating it. This build runs a little slower, its figures
  are only used as shares of the search.
- **Node counts**: a `git archive` copy of the tree with counters added at each point of interest, the header force
  included with `CXX="g++ -include prof_counters.hpp"`. Bench and nodes stayed the same.
- **PGO**: `CXX="g++ -fprofile-generate -fprofile-update=single"`, a training run, delete the `.o` files and keep the
  `.gcda`, then `CXX="g++ -fprofile-use -fprofile-correction"` with the same `BUILD_DIR`. Compared with `test/perf.sh`.
- **History**: `git archive` of each tag, built with its own `ob_build/makefile`, run in turns under one file name since a
  path's length alone shifts alignment.

A sample lands near the instruction that was slow rather than on it, so single source lines are less certain than functions.

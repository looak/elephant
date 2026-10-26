# Test pipeline

Repo: [looak/elephant](https://github.com/looak/elephant), `test/` & `hooks/` · Python 3 (standard library only, but
for the phant-indexer check), POSIX sh · true at the commit it is read at

## Purpose

Turns a build into a run of deterministic checks that nyayo can keep: plays EPD suites through the engine over UCI,
records every position's result with what was built and from which commit, compares it with the run it descends
from, and uploads it. It's the local gate before a merge (smoke) and the deeper check before a merge or release
(extended).

| Piece | Does |
|:--|:--|
| `test/smoke.sh` (`smoke.ps1` on Windows) | the gate: unit tests, bench, ElephantSuite, phant-indexer against python-chess, EPD smoke tier, record, compare, sync |
| `test/epd_runner.py` | plays a tier's checks with any UCI engine, writes one JSON line per check |
| `test/epd_tiers.json` | the tiers: which EPD files, limits, threads, baselines & minimums |
| `test/indexer_smoke.py` | checks phant-indexer against python-chess over fixtures, random games and real games |
| `test/results.py` | records checks as a run, keeps history, compares, uploads to nyayo |
| `hooks/prepare-commit-msg` | the `bench N nodes` line in commit messages |
| `hooks/post-commit` | promotes wip runs to the new commit and syncs |
| `test/RESULTS.md` | the long form of all of this, the record format by example |

## Scope

**Does:** deterministic, repeatable checks run on the developer's machine, with history along the commit graph.

**Doesn't:** Elo or speed (OpenBench), live games (nyayo shows them, uwanja plays them), CI results. CI (`ci.yml`)
runs only the unit tests and perft, correctness checks whose results are thrown away; no EPD tier runs in CI.

## Current

### Smoke, `test/smoke.sh [build dir]`

Runs in order and prints a summary, exit non-zero if a step fails. Run it in a visible console, it takes about a
minute.

1. **Unit**: `ElephantTest`.
2. **Bench**: the makefile build with g++ (WSL on Windows), compared with the last commit's `bench` line. A different
   count warns: fine for a search change, not for a refactor.
3. **Suite**: all of `ElephantSuite`, perft on every core and the deep searches ([tests.md](tests.md)).
3b. **phant-indexer**: `indexer_smoke.py` against python-chess, below. It warns, and doesn't fail, when phant-indexer
   isn't found in the build directory (configure with `-DELEPHANT_BUILD_INDEXER=ON`), when Python has no
   python-chess, or when it has no zstd.
4. **EPD smoke tier** through `epd_runner.py` with `ElephantGambit`.
5. **Results**: `record --kind smoke`, `compare`, `sync`.

### phant-indexer against python-chess

`python test/indexer_smoke.py --indexer <phant-indexer> [--random N] [--seed S] [--keep FILE] [--games DIR]
[--limit N] [FILE...]`, with python-chess installed (`pip install chess`) and zstd for Python (3.14's
`compression.zstd`, or `zstandard`). It exits 0 when every game agrees, 1 on a disagreement, 2 when it couldn't run.
It is not recorded in a results run: pass or fail, in the smoke summary and `logs/`.

Its games are the files given (`test/pgn/*.pgn` when none are: OpenBench's workload as uploaded and cutechess-ob's
games before it strips them, a fastchess gauntlet, edge cases and python-chess's quirks), `--random` games (2000 in
smoke) played by python-chess at random from the start or from a random position, with comments, NAGs and variations
sprinkled in, which promote, castle and capture en passant far more often than engines do, and with `--games DIR`
the `*.pgn` files under a directory, real games. For each game it compares what python-chess reads with
phant-indexer's `--verbose` entry: whether it read and where it stopped, the moves in UCI notation, the final
position, and every fact in the entry. It also compares:

- every record `--positions` writes with python-chess's `chess.polyglot.zobrist_hash` of that position and the move
  played from it, packed the way the engine packs it (the test rebuilds that from python-chess's move), for exactly
  the games that read;
- `--draw 10 --quiet --min-ply 16`: every line a position of a game that read, past ply 16, not in check, labelled
  with the result of a game that reached it, none twice, at most 10 a game, and the same output run twice; with
  `--merge`, each once with a result between the lowest and the highest of the games that reached it, and the same
  twice; with `FILE:P`, positions the games had that many plies in; and `--stats`, whose game counts are
  python-chess's and whose numbers add up;
- `--position` for the first game's start position and six positions picked from each file with the games and plies
  python-chess finds;
- the entries from each file read bz2, gzip and zstd compressed on standard input, zstd both in one frame and in a
  frame every 64 games.

It prints both readers' speed. What phant-indexer reads and writes is in
[engine.md](engine.md#phant-indexer-the-game-reader).

### Tiers (`test/epd_tiers.json`)

A tier is a list of checks; a check is EPD files, one limit (`nodes`, `depth` or `movetime`; nodes are for all
threads together), threads, hash, `baseline` (below it warns) and `minimum` (below it fails). `per_file` takes the
first N positions of each file, `scored` totals STS `c0` points instead of solves, `repeatable: false` (multi
threaded) checks only the minimum. A check's `id` names the suite (`epd.wac_new`), its `name` the check.

| Tier | Checks | Takes |
|:--|:--|:--|
| smoke | WAC 150k nodes, WAC 4 threads 400k, STS 25 per theme 100k (scored), Regression depth 23 (all must pass, REG.003 is found at 22) | ~10 s |
| extended | WAC 4M, WAC 4 threads 16M, Arasan2024 8M, Arasan2024 4 threads 32M, STS full 1M (scored) | ~4 min on 24 cores, local & by hand only |

The extended baselines are targets the owner keeps on purpose, so their warnings are expected.
`res/regression.epd` holds positions the engine once got wrong; a fixed bug adds a line (`bm`/`am` in SAN, the next
`REG.NNN` id, a `c1` note on its origin) and raises the check's baseline & minimum.

### epd_runner.py

`python test/epd_runner.py --engine <binary> --tier smoke|extended [--check NAME] [--jobs N] [--leave N]
[--low-priority] [--out checks.jsonl] [--quiet]`. Experiments override every check: `--nodes`, `--depth`,
`--movetime`, `--threads`, `--hash` (their levels don't apply then).

Each position is its own game (`ucinewgame`, `position fen`, `go <limit>`), spread over one engine process per core
(`--jobs`, default all logical cores less `--leave`; a 4-thread check gets jobs / 4 engines). SAN is matched against
the engine's UCI move without a move generator, so any UCI engine works. Before searching it asks `buildinfo` (see
[engine.md](engine.md#uci)). Per check it writes params, metrics (`solved`, `total`, `points`,
`max_points`, `duration`), the engine's verdict, the build, `started_at`/`finished_at`, and every case with the move
played, score, nodes, `time_ms` and PV.

### results.py

`record`, `sync`, `compare [RUN] [--against RUN] [--previous]`, `case ID`, `list`. The store is `logs/results/`
(per machine, ignored by git): `history/` and `wip/`.

- **States**: a run on a dirty tree is **wip** (it records the tree the working copy would commit as); once a commit
  with exactly that tree exists it's **committed**; once that commit is on a remote branch it's **published**. Only
  published runs are permanent. `sync` (from smoke and the post-commit hook) moves runs forward and uploads.
- **Refusals**: `record` refuses checks from an engine built from another commit than HEAD, or from a clean tree when
  the working tree has changed since.
- **Comparison** follows ancestry: the newest comparable run on the nearest ancestor commit (300 commits of ancestry
  travel with each run), so parallel branches don't mix. Comparable means the same check id & version, limit,
  threads, hash, experiment flag and build (compiler, config, assertions, exceptions, bits).
- **Upload**: `PUT {nyayo.url}/api/runs/{id}`, gzipped, `Authorization: Bearer {nyayo.token}`, from git config
  `nyayo.url` & `nyayo.token` or `NYAYO_URL` & `NYAYO_TOKEN`. Without them runs stay local.

### Extended runs

There's no script for the extended tier yet: run `epd_runner.py --tier extended` with `--out`, then
`results.py record --kind extended --started <utc>`, `compare` and `sync`. `--leave 4` keeps the machine usable.

## Interfaces

| Direction | What | Contract |
|:--|:--|:--|
| in | an engine binary speaking UCI (+ `buildinfo`) | [engine.md](engine.md#uci) |
| in | `phant-indexer` and python-chess | [engine.md](engine.md#phant-indexer-the-game-reader) |
| in | EPD files in `res/` | `bm`, `am`, `id`, `c0` (STS points); `c1` free text |
| in | the tiers | [`test/epd_tiers.json`](../../test/epd_tiers.json), which is the contract |
| out | runs to nyayo | nyayo `docs/results-schema.md`, schema v1 |
| out | bench line in commit messages | [engine.md](engine.md#bench) |


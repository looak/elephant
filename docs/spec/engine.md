# Elephant engine

Repo: [looak/elephant](https://github.com/looak/elephant) · C++20 · true at the commit it is read at; `version.txt` is 1.4.0
on `main`, with a bench of 504347

## Purpose

A UCI chess engine, Elephant Gambit. It plays in any UCI GUI or match runner, on OpenBench and on lichess. One engine
library (`src/engine`) is built into two binaries with different jobs, an offline tool tunes its evaluation, and a
fourth binary reads games with the library's PGN and SAN parsing and move generation for nyayo's game catalogue:

| Binary | Built by | Entry | For |
|:--|:--|:--|:--|
| `elephant_ob` and the release binaries | `ob_build/makefile`, g++ | `src/cli/src/openbench.cpp` | playing: OpenBench, GitHub releases, match runners |
| `ElephantGambit` | CMake (MSVC, clang, gcc) | `src/main.cpp` | development: an interactive CLI, UCI with `ElephantGambit uci`, the engine epd_runner drives in smoke |
| `ElephantTuner` | CMake, `ELEPHANT_BUILD_TUNER` (on by default) | `tuner/src/main.cpp` | offline: fits the evaluation tables to a dataset of positions and results, see [Evaluation tables & tuning](#evaluation-tables--tuning) |
| `phant-indexer` | CMake target `PhantIndexer`, `ELEPHANT_BUILD_INDEXER` (off by default) | `indexer/src/main.cpp` | offline: reads PGN and writes one JSON line per game, and every position's hash, see [phant-indexer](#phant-indexer-the-game-reader) |

`ElephantGambit` and `elephant_ob` speak the same UCI. The makefile build is the reference: OpenBench builds it, the
release binaries are it, and the bench signature comes from it. The test binaries are in [tests.md](tests.md).

## Scope

**Does:** play legal chess over UCI with a time, depth, node or mate limit; search with up to 64 threads sharing one
transposition table; report its search as UCI `info`; report what it was built from; produce a deterministic bench.

**Doesn't:** opening books, endgame tablebases, MultiPV, Chess960, advertising `Ponder` (it handles `go ponder` but
doesn't offer the option), any network I/O. phant-indexer reads and replays games, it plays none, and reads no
Chess960.

## Current

### UCI

Standard [UCI](https://www.shredderchess.com/download/div/uci.zip), plus two non standard commands. A command it can't parse never ends the engine: it answers
`info string error <reason>` and carries on.

| Command | Notes |
|:--|:--|
| `uci` | `id name Elephant Gambit <version>`, `id author Alexander Loodin Ek`, the options, `uciok` |
| `isready` | `readyok`, straight away |
| `setoption name <name> value <v>` | names may contain spaces (`Move Overhead`) |
| `ucinewgame` | clears the transposition table and the game's history |
| `position startpos\|fen <fen> [moves ...]` | |
| `go ...` | `depth`, `nodes`, `mate`, `movetime`, `wtime`, `btime`, `winc`, `binc`, `movestogo`, `infinite`, `ponder`, `searchmoves` |
| `stop`, `ponderhit`, `quit`, `debug on\|off`, `register` | `register` is accepted and ignored |
| `bench` | runs the bench, see below |
| `buildinfo` | non standard, see below |
| `print` | non standard, prints the board |

**Options**

| Option | Type | Default | Range |
|:--|:--|:--|:--|
| `Threads` | spin | 1 | 1 – 64 |
| `Hash` | spin, MB | 8 | 1 – 32768 |
| `Move Overhead` | spin, ms | 10 | 0 – 5000 |

**Output.** Every completed iteration of the main thread prints
`info score cp|mate <s> depth <d> nodes <n> time <ms> hashfull <h> pv <moves>`, nodes counted over all threads; the
last line before `bestmove` is `info nodes <n> time <ms> nps <nps>`. With `debug on` the helper threads' iterations
come as `info string thread <id> depth <d> score <s> nodes <n> pv <moves>`.
With no legal move at the root (mate or stalemate) the answer is `bestmove 0000`. `position fen` refuses a board
without exactly one king a side.

**buildinfo.** `buildinfo` answers one `buildinfo <key> <value>` line per key, then nothing; callers send `isready`
right after and take whatever arrives before `readyok`. Engines that don't know the command ignore it, as UCI says,
so the pattern works with any engine. Keys: `version`, `commit` (full hash), `dirty`, `branch` (left out on a detached
HEAD), `compiler`, `config`, `assertions`, `exceptions`, `bits`. They're written on every build
(`cmake/build_info.cmake`, the makefile's rule), so a binary can't claim a commit it wasn't built from. `compiler`,
`config`, `assertions`, `exceptions` and `bits` decide which results are comparable, in nyayo's terms.

### Bench

`elephant_ob bench` (command line) and the UCI `bench` search a fixed list of 26 positions to depth 7 with one
16 MB transposition table reused and cleared between positions. The command line prints `info string <ms> ms`, then
`<nodes> nodes <nps> nps`; the UCI `bench` prints `info string <s> seconds`, then `info string <nodes> nodes <nps> nps`.
The node count is the **bench signature**: it changes when the search or the evaluation changes, and only then. It is
504347 on `main` at 1.4.0. Every commit
message carries `bench <nodes> nodes`, written by `hooks/prepare-commit-msg`: when the commit touches the engine
(`src/engine`, `src/cli/src`, `ob_build/makefile`) the hook runs the bench from the g++ makefile build (through WSL on
Windows), otherwise it repeats the last commit's line. `ELEPHANT_BENCH=force` runs it anyway, `ELEPHANT_BENCH=skip`
leaves the line out. Windows and Linux builds give the same count since the Zobrist keys come from a fixed seed
splitmix64.

### Search

Negamax alpha-beta with PVS, iterative deepening with aspiration windows, Lazy SMP, a lock-free transposition table
of 8 entries per 64-byte bucket, quiescence with delta & SEE pruning, check extensions, null move pruning with
verification, LMR, LMP, IIR, reverse futility and futility pruning; ordering by PV/TT move, MVV-LVA, killers, history,
losing captures last. Threefold repetition (with the game's earlier positions) and the fifty move rule. Time management
uses soft & hard limits, is increment aware and subtracts `Move Overhead`. The README's feature list is the long form.

### Evaluation

One static evaluation from white's side, turned to the side to move's by search. Every term is a `TaperedScore`, a
midgame and an endgame value packed into one 32-bit integer and written `S(mg, eg)` (`eval/tapered_score.hpp`). The
terms are summed packed and tapered once, by `evaluator_data::taper` in the same file, on PeSTO's game phase: knights
and bishops 1, rooks 2, queens 4, 24 with all pieces on the board = all midgame, 0 = all endgame, capped at 24 after
promotions.

| Term | Code | Values | Counted |
|:--|:--|:--|:--|
| material & piece square tables | `eval/pesto_accumulator.hpp` | `eval/pesto_tables.hpp`: PeSTO's, the pawn table retuned | incrementally by make/unmake |
| mobility: safe squares of knights, bishops, rooks & queens | `eval/mobility.hpp` | `eval/mobility_tables.hpp`, tuned | every evaluation |
| pawn structure: passed, isolated, doubled, backward, connected | `eval/pawn_structure.hpp` | `eval/pawn_tables.hpp`, tuned | cached by the pawns, see below |
| passed pawn extras: own & enemy king distance, a piece on the stop square, a free path to promotion, a rook behind | `eval/passed_pawns.hpp` | `eval/pawn_tables.hpp`, tuned | every evaluation |
| king shelter & pawn storm: on the king's file and the two beside it, its own nearest pawn (shelter) and the enemy's (storm, blocked or not) | `eval/king_safety.hpp` | `eval/king_tables.hpp`, tuned | every evaluation |
| king attack: the enemy's knights, bishops, rooks & queens on the king zone, the files open to its rooks & queens | `eval/king_safety.hpp` | `eval/king_tables.hpp`: the danger tuned, the units hand-set | every evaluation, counted in mobility's pass |
| piece pairs: bishops on both colours, two or more knights, two or more rooks | `eval/pieces.hpp` | `eval/piece_tables.hpp`, tuned | every evaluation |
| mop-up: more than two pawns ahead in an endgame (12 pieces or fewer, or under half the starting material counting six rooks), the winning king closes in on the enemy's and to the centre | `eval/mop_up.hpp` | `evaluator_data::mopUpKingDistance`, `mopUpCentre`, hand-set | every evaluation, outside the taper |
| tempo: 15 cp to the side to move | `search/search_threadcontext.hpp` | `evaluator_data::tempoBonus`, hand-set | by search, after the turn to the side to move's view |

`Evaluator::EvaluateTapered` is the tapered sum, the part the tuner fits; `Evaluate` adds mop-up and asserts both
kings are on the board (search never evaluates without one), search adds tempo. A safe square is
one a piece attacks that holds none of its own pieces and isn't attacked by an enemy pawn. Pawn terms are per pawn,
from its own side: passed and connected by relative rank 2–7, isolated, doubled and backward by file folded onto a–d
(a & h share a value), the extras by king distance 0–7 or relative rank. The structure depends on the pawns alone:
`pawnInfo` returns its score and each side's passed pawns, and each search thread keeps a pawn hash
(`eval/pawn_hash.hpp`, 2^14 entries) caching that by both pawn bitboards, an exact match. The extras depend on the
pieces and read the cached passed pawns. The tuner and one off evaluations go without the cache; it changes no node
count.

King shelter & storm look at the king's file and the two beside it, moved in from the edge, and only at pawns on the
king's rank or in front of it: the king's own nearest pawn by relative rank or none, the enemy's nearest by how far it
has come, scored from its own table by rank when the king's pawn blocks it, all by file folded onto a–d. King attack
counts, in mobility's pass (`KingZoneAttacks`, `eval/mobility.hpp`), the enemy pieces reaching the king zone (the
king's square, the squares around it and those a rank further towards the enemy) and their units per zone square
reached (`kingAttackUnits`: knight 2, bishop 2, rook 3, queen 5); with an enemy rook or queen each of the three files
adds 3 when open and 2 without the king's pawns. Nothing counts below two attackers; the score is
`kingDanger[min(units, 63)]`.

### Evaluation tables & tuning

The tables are plain `constexpr` arrays of `S(mg, eg)` in `src/engine/inc/eval/*_tables.hpp`, compiled in. Nothing
reads weights at runtime: there is no weights file, no tuning build and no option that changes them. A new value
reaches the engine as a changed header in a commit, which moves the bench.

| File | Tables | Values | Status (1.4.0) |
|:--|:--|--:|:--|
| `pesto_tables.hpp` | `pestoMaterial`, `pestoPawn` … `pestoKing` (by square, A8 first) | 6 + 384 | PeSTO's material and king table; the pawn to queen tables rebalanced with every term, each kept centred on its own average |
| `mobility_tables.hpp` | `knightMobility`, `bishopMobility`, `rookMobility`, `queenMobility` (by safe squares, 0 first) | 66 | rebalanced with every term, each table centred at 0 |
| `pawn_tables.hpp` | `passedPawn`, `connectedPawn` (by rank), `isolatedPawn`, `doubledPawn`, `backwardPawn` (by folded file); `passedOwnKing`, `passedEnemyKing` (by distance), `passedBlocked`, `passedFreePath` (by rank), `passedRookBehind` (own, enemy) | 24 + 30 | rebalanced with every term |
| `king_tables.hpp` | `kingShelter`, `kingStorm` (by folded file × no pawn & rank), `kingStormBlocked` (by rank); `kingDanger` (by attack units 0–63); the hand-set `kingAttackUnits`, `kingOpenFileUnits`, `kingHalfOpenFileUnits` | 72 + 64 | shelter & storm rebalanced with every term; the danger tuned in 1.3, its sparse tail made monotonic, kept since |
| `piece_tables.hpp` | `piecePair` (knight, bishop, rook) | 3 | rebalanced with every term |

Every table rebalanced with every term was then fine-tuned on Elephant's own games, the same groups, kept values and
centring (1.4.0-pieces.3).

**ElephantTuner** (`tuner/`, its README has the long form) fits these tables to a dataset, one position per line as a
FEN and the game's result. Phase 0 plays each position's quiescence line out to a quiet position, evaluates it with a
trace that records every table entry it used and how often, and checks that the trace rebuilds the engine's
`EvaluateTapered` exactly; mop-up is carried along as a fixed offset. Phase 1 finds K, the scale of the sigmoid that
turns an evaluation into an expected result. Phase 2 fits the values by gradient descent with Adam against the
results, every 20th position held back to check the fit with. The output is the engine's own headers with only the
numbers replaced, written as `*.tuned.hpp`; copying one over the original is the change. `--epochs 0` writes the
values back untouched and checks the files come out byte for byte the same.

The values are tuned in groups, `--tune` naming the ones that move while the rest stay as they are: `material`,
`psqt_pawn` … `psqt_king` (`psqt` for all six), `mobility`, `pawns_passed`, `pawns_structure`, `pawns_passed_extras`, `king_shelter`, `king_attack`, `pieces_pairs`.
The practice so far is to tune only the new group with everything else kept, then SPRT it, and at the end of a cycle
the piece square table the new terms overlap together with them. Fitted together, the tables re-price the pieces: with
material free its midgame values fell by a fifth, with material fixed the piece square tables moved their averages
instead, by up to 175 centipawns in the endgame. So the tuned piece square tables are kept centred on the average each
starts with, as mobility's are at 0 (`--free-psqt` turns it off), and `--min-usage N` keeps the values fewer than N
positions use as they are. Tables that are rare in the data mislead: fitted with
the pawn terms, the king table learned large midgame bonuses for an advanced king, a king that far up with the queens
on being mostly a game already won, and was left out; the danger table's tail past 48 units, where few positions are,
rose back towards 0 and was made monotonic by hand. The fit's held back error is a weak guide to Elo: king attack's
0.61% gave less at short time control than shelter's 0.38%, both passed at long time control. The datasets are in `res/tuning_positions/`; every table above was fitted on
`E12.52-1M-D12-Resolved` (10M positions), the one to keep using while groups are added one at a time, since the kept
values are only best for the data they were fitted on. `elephant-positions-0.1` holds 1.2M positions from Elephant's
own games in nyayo (FEN and result, no duplicates); the 1.4 fine-tune fitted on it lowered its held back error 1.23%
and raised E12.52's 1.13%, yet passed in play: the two datasets disagree, and the positions Elephant reaches won.

### ElephantGambit, the development binary

Without arguments it starts an interactive CLI: `help`, `about`, `new`, `fen`, `move`, `print`, `evaluate`, `search`,
`perft`, `divide`, `bench`, `exit`. `ElephantGambit uci` goes straight into UCI mode. It logs through spdlog. CMake
builds have assertions and exceptions on in every configuration by default (`ASSERTIONS_ENABLED`,
`EXCEPTIONS_ENABLED` in `cmake/options.cmake`, always on in Debug); the makefile builds have neither.

### phant-indexer, the game reader

`indexer/` builds `phant-indexer`, the reader of nyayo's game catalogue, from the engine library's `io` code (the PGN
tokenizer and game reader, the SAN parser, FEN) and its move generation. It reads PGN, plain or bz2, gzip or zstd told
by its first bytes, from files or standard input, and writes one JSON line per game ("a card") to standard output:
whether every move read and was legal (else the ply and the reason), the tags, the plies, how the game ended by
the rules, promotions, castling, captures, checks, the fewest pieces and material at fixed plies, and the Polyglot
hash of the start position and of the position after 16 plies. It keeps no state between runs and maps no names:
whoever plays the games writes the players and tags.

- **Reads as python-chess does.** The PGN rules (how games split, tags, movetext, null moves, the result) are
  `chess.pgn.read_game`'s, which nyayo reads games with today, and a SAN move is resolved against the legal moves. The
  one difference: a `Variant` that isn't standard chess is an error, Elephant doesn't play Chess960.
- **Positions.** A position is its Polyglot hash, the standard one (the 781 numbers of Polyglot's book format, not the
  engine's own Zobrist keys), which python-chess computes as `chess.polyglot.zobrist_hash`. `--positions FILE` writes
  12 byte little-endian records, the hash, the game's place in the input and the ply, for every position of every game
  that read, in the same pass as the cards; it takes one input and stops at 65 535 games or plies, the 2 byte fields'
  limit, saying so and exiting 1. `--position FEN` writes the games and plies that reach a position instead of the
  cards.
- **Built only with `ELEPHANT_BUILD_INDEXER`**, off by default, and with `EXCEPTIONS_ENABLED`: the parsers report an
  illegal move by throwing. With it on, configuring fetches zlib, bzip2 and zstd at pinned tags and builds their
  decompression as static libraries linked into phant-indexer alone, so the release binary needs nothing installed. An
  engine build, and `ob_build/makefile`, which never builds phant-indexer, fetch and link none of it; the bench doesn't
  move.
- **The cards' format** (now format 2) and the positions records are a contract with nyayo, owned here:
  [`indexer/README.md`](../../indexer/README.md) is the text. Its tests are in [tests.md](tests.md#phant-indexer), its
  check against python-chess in [test-pipeline.md](test-pipeline.md#phant-indexer-against-python-chess).

### Builds & releases

- `ob_build/makefile`: `g++ -std=c++2a -O3 -flto=auto -march=$(ARCH)`, `ARCH` defaults to `native`. OpenBench runs it
  with its own `EXE`.
- `release.yml`, on publishing a GitHub release (or by hand): the makefile for Windows & Linux at `x86-64`,
  `x86-64-v2` and `x86-64-v3`, linked static, named `elephant-gambit-<version>-<os>-<arch>`; and, in a job of its own
  with CMake and `ELEPHANT_BUILD_INDEXER=ON`, phant-indexer for Windows & Linux at `x86-64`, linked static (the C
  runtime too), named `phant-indexer-<version>-<os>-x86-64` and smoke tested on OpenBench's workload, bz2 compressed,
  before it is attached.
- `version.txt` is the only place the version is written. major = engine generation or a UCI option change, minor =
  a cycle with an SPRT verified gain or feature, patch = fixes and non-regressions. Pre-releases are
  `<version>-<label>.N`, e.g. `1.0.0-herd.4`. Tags are lightweight.
- A cycle has its own branch off `main` (`eval-pawns` for 1.2), each change a branch `<cycle>-<change>` off it, SPRT'd
  against the cycle's tip and merged on a pass; `version.txt` on the cycle branch counts the merges
  (`1.2.0-pawns.1`). `main` takes a cycle when it is released, and work that doesn't move the bench (cleanups, the
  tests, phant-indexer) by pull request, as a patch version when it is released.

## Interfaces

| Direction | What | Contract |
|:--|:--|:--|
| in | UCI on stdin | above |
| out | UCI on stdout | above |
| out | bench line in commit messages | `bench <nodes> nodes`, its own paragraph before trailers |
| out | release binaries | `release.yml` |
| in | PGN, plain, bz2, gzip or zstd, on standard input or as files | phant-indexer, read as python-chess reads it, [`indexer/README.md`](../../indexer/README.md) |
| out | cards, JSON lines, format 2; position records; `--position` lines | phant-indexer, [`indexer/README.md`](../../indexer/README.md) |


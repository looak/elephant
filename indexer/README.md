# phant-indexer

Reads PGN and writes one JSON line per game: whether every move read and was legal, and facts about the game that
need it replayed; with `--positions`, every position's hash too, which is how nyayo finds games by position. It's the game reader of nyayo's game catalogue (ramani, `plans/game-database.md`), replacing
python-chess there, built on Elephant's PGN and SAN parsing and its move generation. It only reads: the players and
the `Subject` and `Opponent` tags are written by whoever plays the games (OpenBench's worker writes them before it
uploads), nothing is mapped here.

```
phant-indexer [--verbose] [FILE...]                 the games of each FILE, or of standard input when there's none or FILE is -
phant-indexer --positions OUT [--verbose] [FILE]    the entries, and every position's record to OUT; one input
phant-indexer --position FEN [FILE...]              the games & plies that reach FEN's position, instead of the entries
phant-indexer --draw N [--quiet] [--min-ply P] [--seed S] [FILE...]    training positions, instead of the entries
phant-indexer --version                             phant-indexer <elephant version> format <format version>
```

An input is plain PGN, bz2 (as an OpenBench worker uploads a workload), zstd (as nyayo stores its batches) or gzip,
told by its first bytes, and may be several compressed streams one after the other: zstd frames, skippable frames
among them, bz2 or gzip streams. Offsets and lengths are into the decompressed PGN. Exits 0 when every
input could be read, a game that doesn't read is an entry, not an error; 1 when an input can't be read or
decompressed (nothing of that input is written) or `--positions` meets more than it can number (below); 2 on a bad
option or a FEN that doesn't read. It keeps no state between runs.

Built by CMake as the `PhantIndexer` target (`build/Release/bin/phant-indexer`) only with `ELEPHANT_BUILD_INDEXER`
on (off by default) and `EXCEPTIONS_ENABLED` on, the parsers report an illegal move by throwing:

```
cmake -B build -DELEPHANT_BUILD_INDEXER=ON
```

bz2, gzip and zstd are bzip2 1.0.8's, zlib 1.3.1's and zstd 1.5.7's own sources, their decompression only, fetched at
configure time and linked into phant-indexer alone; an engine build, the default, fetches none of them, and
`ob_build/makefile` never builds phant-indexer. Its tests join `ElephantTest` when it's built. Released as
`phant-indexer-<version>-<linux|windows>-x86-64`, linked static, by `release.yml`, which turns the option on, as CI's
`unit` job does.

## An entry, format 2

The entries and the positions output (below) are the reader's contract with nyayo, proposed and not agreed yet
(ramani, `plans/game-database.md`, open decisions 9 and 15). A field added or changed means a new format version.

```jsonc
{
  "format": 2,                    // the version of these fields
  "reader": "1.3.1",              // the Elephant version phant-indexer was built from
  "source": "batches/7.pgn",      // the file, only when files were given
  "game": 17,                     // the game's place in its input, from 1
  "offset": 54210,                // where it starts in its input, in bytes
  "length": 3311,                 // its length in bytes, through its movetext
  "ok": true,                     // every move read and was legal
  "tags": { "White": "Elephant 1.3.0", "FEN": "...", "Result": "1-0" },   // as written, see below
  "result": "1-0",                // the Result tag, unless it's * or missing: then the movetext's first result, or "*"
  "plies": 143,                   // the moves played, null moves included; up to the error when not ok

  // when ok:
  "ended": "checkmate",           // checkmate, stalemate, insufficient_material, fifty_moves, repetition, or null
  "start": "463b96181691fc9c",    // the start position's Polyglot hash, 16 lowercase hex digits
  "opening": "a93e4c07d1b2f685",  // the position's after 16 plies, or null in a shorter game
  "promotions": { "q": 1, "r": 0, "b": 0, "n": 1 },
  "castled": { "white": 9, "black": null },     // the ply of the castling move, counted in this game
  "captures": 27,                 // en passant captures included
  "ep_captures": 1,
  "checks": 11,                   // moves that gave check
  "min_pieces": 6,                // the fewest pieces on the board, kings included, the start position's counted
  "material": { "20": 0, "40": 1, "end": 5 },   // white's less black's, P 1, N B 3, R 5, Q 9; after plies 20 & 40 when reached

  // when not ok, instead of the facts:
  "error": { "ply": 7, "reason": "Ke3: [IO Error] san_parser :: No matching moves found for SAN: e3" },

  // with --verbose:
  "moves": ["e2e4", "e7e5"],      // UCI, castling as the king's move, a null move 0000
  "fen": "..."                    // the final position, or where it stopped
}
```

- `ended` is what the final position says by the rules, checked in that order. A game that ended by resignation,
  adjudication, time or a crash is `null`; the `Termination` tag, where the PGN has one, tells those apart.
- `error.ply` is the ply that didn't read, from 1, or 0 for an error before the moves: a `FEN` tag that doesn't read,
  or a `Variant` that isn't standard chess.
- The plies are counted in the game, from its `FEN` tag when it has one, not by the FEN's move number.

**Format 1** (elephant#28) is format 2 without `start` and `opening`, the version a reader of either has to tell
apart by `format`; nothing else differs.

## Positions

A position is its **Polyglot hash**, the standard one (the 781 random numbers of Polyglot's book format, in
`src/polyglot_keys.hpp`, not the engine's own Zobrist keys), which python-chess computes as
`chess.polyglot.zobrist_hash`: the pieces, the side to move, a castling right only while the king and that rook are
on their squares, and the en passant file only when a pawn of the side to move stands next to the pawn that double
pushed, legal capture or not. Not the move counters, so a position reached by different moves at different points of
a game is the same position. The card's `start` and `opening` are this hash.

**`--positions OUT`** writes, in the pass that writes the entries, a record for each position of each game that
read: the start position (ply 0), then the position after each move, null moves included, in game order. A game that
didn't read writes none. A record is 12 bytes, little-endian, one after the other with nothing else in the file:

| Bytes | Field |
|:--|:--|
| 0–7 | the Polyglot hash, u64 |
| 8–9 | the game's place in its input, from 1, as the entry's `game`, u16 |
| 10–11 | the ply, 0 for the start position, u16 |

The records name a game by its place in its input only, so `--positions` takes one input (a file or standard input).
Two bytes number 65 535 games an input and 65 535 plies a game: a nyayo batch is 64 games, an OpenBench workload a
few hundred, but a larger input (an upload of many workloads, a backfill) can pass it. Then phant-indexer writes the
entries and records of the first 65 535 games, stops at the next game that read, says so on standard error and
exits 1, and the caller splits the input. No game reaches 65 535 plies but by thousands of null moves.

**`--position FEN`** reads the input like the entries do and writes, instead of them, one JSON line for each game and
ply whose position has FEN's hash, in game order, `source` included when files were given:

```jsonc
{"game": 7, "ply": 4}
```

It is the oracle a filter built from the records is checked against: the same games, the same plies. FEN's move
counters are ignored, as the hash ignores them. Only games that read are searched.

## Training positions

**`--draw N`** reads the games like the entries do and writes, instead of them, training positions for the evaluation's
tuner: one `<fen> [result]` line each, the result white's, `[1.0]`, `[0.5]` or `[0.0]`, the format `ElephantTuner`
reads (`tuner/src/dataset.cpp`; E12.52 is in it too). The input is nyayo's export or any batch: zstd, bz2, gzip or
plain PGN, several files if given, one run. For each game that read and has a result (a `Result` tag or the movetext's
of `1-0`, `0-1` or `1/2-1/2`):

- **`--min-ply P`** skips the first P plies: a position is a candidate from the one after the Pth ply on (16 skips an
  eight move book). Default 0.
- **`--quiet`** keeps only the candidates that are *naturally quiet*: the side to move is not in check, and Elephant's
  own quiescence search, run on the position, finds nothing it would rather play than stand pat, so the search returns
  the position's own static evaluation. The search is only a test. Its line is thrown away, and **every position
  written is one the game reached**, as it stood on the board, FEN counters included. Without `--quiet` every
  candidate that has a legal move and is not in check counts.
- It samples up to **N** of the candidates a game has, a seeded shuffle and the first N, so a game with fewer gives
  fewer. **`--seed S`** (default 1) repeats a run: the same inputs in the same order, options and seed give the same
  lines on any platform. A game's sample doesn't depend on the other games'.
- A position is written **once in a run**, by its Polyglot hash, with the result of the first game that gave it. A
  later game's duplicates are skipped and the next candidates taken, so a game can still give N.

A game that didn't read, one without a result and one with a null move (never played over the board) give nothing.
The positions of a checkmate or stalemate aren't written. A summary goes to standard error: the games, the positions
looked at and the naturally quiet among them, how many a game keeps (mean, minimum, 5th percentile, median, and how
many games have fewer than N), and the duplicates left out. `--draw` doesn't go with `--positions`, `--position` or
`--verbose`; `--quiet`, `--min-ply` and `--seed` go with `--draw` alone.

```
phant-indexer --draw 10 --quiet --min-ply 16 eg-1.4.pgn.zst > EG-1.4-own.book
ElephantTuner EG-1.4-own.book --tune psqt_king
```

On 2026-10-26, on nyayo's 730 stored batches (45,088 games, from a rehearsal copy): 4,334,887 positions from ply 16
with a legal move and not in check, 3,500,769 of them naturally quiet; a game keeps 77.6 on average, 65 at the median,
18 at the 5th percentile, none at the minimum, and 773 games (1.7%) have fewer than 10. With `--draw 10`, 443,761
positions were written (13,086 duplicates left out), 136,250 won by white, 189,056 drawn and 118,455 won by black, in
10 seconds on one thread. `ElephantTuner` reads all of them, and its own quiescence search finds every one already
quiet (443,761 already quiet, 0 resolved).

## Read as python-chess reads it

Every choice below is `chess.pgn.read_game`'s, which nyayo reads games with today, so the two agree game for game:

- **Games** are split where python-chess splits them: `;` and `%` lines are skipped before and among the tags, one
  blank line among the tags or before the movetext is skipped, a second ends the game without moves, and a blank
  line in the movetext (outside a comment, inside a variation too) ends the game; what follows is the next one.
- **Tags**: a value runs from the first quote to the last `"]` of its line and is kept as written, `\"` included; a
  line that isn't a tag is skipped. A name given twice is kept once, at its first place, with its last value. The
  seven tag roster isn't filled in: `tags` is what the PGN has.
- **Movetext**: move numbers, NAGs, `!?` suffixes, variations and `%` lines are skipped, `{}` and `;` comments are
  read past. A result doesn't end the moves: those after it are played. `--`, `Z0`, `0000` and `@@@@` are null moves:
  the side to move passes, no en passant capture is left, the fifty move count goes on.
- **SAN** is resolved against the legal moves: piece, file, rank or square disambiguation, `x` or not, `=Q` or `Q`,
  `O-O` and `0-0`, `+` and `#` ignored. Nothing after an illegal or ambiguous move is read.
- **Not python-chess's**: a `Variant` other than standard chess (python-chess's names for it: Standard, Chess,
  Classical, Normal, Illegal, From Position, in any case) is an error before the moves. Elephant doesn't play
  Chess960, which python-chess reads.

## Checked against python-chess

`test/indexer_smoke.py` reads games with phant-indexer and python-chess and compares every game's moves, final
position and entry. Its games are `test/pgn/*.pgn` (OpenBench's workload as uploaded and cutechess-ob's games before
OpenBench strips them, a fastchess gauntlet, edge cases, python-chess's quirks), random games played by python-chess,
which promote, castle and capture en passant far more often than engines do, and with `--games DIR` real games. For
`--draw` it checks that every position written is a position of a game that read, from the 16th ply on, not in check,
labelled with the result of a game that reached it, once in the run, at most 10 a game and the same on a second run. Each
file is read bz2'd, gzipped and zstd'd on standard input too, zstd in one frame and in a frame every 64 games, and has
to give the same entries. Every record `--positions` writes has to be python-chess's `zobrist_hash` of that position,
for exactly the games that read, and `--position` for the first game's start position and six positions picked from
each file has to find exactly the games and plies python-chess finds. It needs python-chess and zstd for Python
(3.14's `compression.zstd`, or `zstandard`). `test/smoke.sh` runs it with 2000 random games. The entries' format is
pinned by `test/src/indexer/game_index_test.cpp`, the hash by Polyglot's own test vectors in `polyglot_test.cpp`.

On 2026-10-21: 20 068 random and fixture games agree, 3 617 137 position hashes and 49 scans with them, and 4 253
real ones (uwanja's CCRL blitz gauntlet and nyayo's live games, fastchess's output with PVs in the comments), 584 555
position hashes and 7 scans. On OpenBench's workload repeated (10 200 games), one thread of a Ryzen 9 9900X, the
Linux release build, best of three:

| Input | `--positions` off | on |
|:--|--:|--:|
| plain on standard input | 27 950 games/s | 24 000 games/s |
| zstd on standard input | 32 400 games/s | 27 400 games/s |

zstd is faster than plain here because a tenth of the bytes cross the pipe. bz2 reads 13 100 games a second;
python-chess's `read_game` reads 700.

## Not built yet

- The eval at fixed plies and its largest swing, from the comments.
- Several files at once on several threads (`--jobs`).

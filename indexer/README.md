# phant-indexer

Reads PGN and writes one JSON line per game: whether every move read and was legal, and facts about the game that
need it replayed. It's the game reader of nyayo's game catalogue (ramani, `plans/game-database.md`), replacing
python-chess there, built on Elephant's PGN and SAN parsing and its move generation. It only reads: the players and
the `Subject` and `Opponent` tags are written by whoever plays the games (OpenBench's worker writes them before it
uploads), nothing is mapped here.

```
phant-indexer [--verbose] [FILE...]       the games of each FILE, or of standard input when there's none or FILE is -
phant-indexer --version                   phant-indexer <elephant version> format <format version>
```

An input is plain PGN, bz2 (as an OpenBench worker uploads a workload), zstd (as nyayo stores its batches) or gzip,
told by its first bytes, and may be several compressed streams one after the other: zstd frames, skippable frames
among them, bz2 or gzip streams. Offsets and lengths are into the decompressed PGN. Exits 0 when every
input could be read, a game that doesn't read is an entry, not an error; 1 when an input can't be read or
decompressed (nothing of that input is written); 2 on a bad option. It keeps no state between runs.

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

## An entry, format 1

The format is a contract with nyayo, proposed and not agreed yet (ramani, `plans/game-database.md`, open decision 9).
A field added or changed means a new format version.

```jsonc
{
  "format": 1,                    // the version of these fields
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
which promote, castle and capture en passant far more often than engines do, and with `--games DIR` real games. Each
file is read bz2'd, gzipped and zstd'd on standard input too, zstd in one frame and in a frame every 64 games, and has
to give the same entries. It needs python-chess and zstd for Python (3.14's `compression.zstd`, or `zstandard`). `test/smoke.sh` runs it with
2000 random games. The entries' format is pinned by `test/src/indexer/game_index_test.cpp`.

On 2026-10-20: 20 068 random and fixture games agree, and 4 253 real ones (uwanja's CCRL blitz gauntlet and nyayo's
live games, fastchess's output with PVs in the comments). On OpenBench's workload repeated, one thread of a Ryzen 9
9900X, the Linux release build reads 27 800 games a second plain, 26 300 from zstd and 13 100 from bz2;
python-chess's `read_game` reads 700.

## Not built yet

- The other facts of the index card: the start position and the position after 16 plies as hashes, and the eval at
  fixed plies & its largest swing from the comments. The hashes are best a standard one (Polyglot's), which nyayo can
  compute in Python too, not the engine's own Zobrist keys.
- Several files at once on several threads (`--jobs`).

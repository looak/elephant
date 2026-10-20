# phant-indexer

Reads PGN and writes one JSON line per game: whether every move read and was legal, and facts about the game that
need it replayed. It's the game reader of nyayo's game catalogue (ramani, `plans/game-database.md`), replacing
python-chess there, built on Elephant's SAN parser and move generation.

```
phant-indexer [--verbose] [FILE...]       the games of each FILE, or of standard input when there's none or FILE is -
phant-indexer --version                   phant-indexer <elephant version> format <format version>
```

Built by CMake as the `PhantIndexer` target (`build/Release/bin/phant-indexer`), when `EXCEPTIONS_ENABLED` is on:
the parsers report an illegal move by throwing. Exits 0 when every input could be read, a game that doesn't read is
an entry, not an error; 1 when an input can't be read, 2 on a bad option.

## An entry, format 1

```jsonc
{
  "indexer": "1",                 // the format version, bumped when a field is added or changes
  "source": "batches/7.pgn",      // the file, only when files were given
  "game": 17,                     // the game's place in its input, from 1
  "offset": 54210,                // where it starts in its input, in bytes
  "length": 3311,                 // its length in bytes, through its result
  "ok": true,                     // every move read and was legal
  "tags": { "White": "Elephant 1.3.0", "FEN": "...", "Result": "1-0" },   // as written, in order
  "result": "1-0",                // the Result tag, else the movetext's result, else "*"
  "plies": 143,                   // the moves played; up to the error when not ok

  // when ok:
  "ended": "checkmate",           // checkmate, stalemate, insufficient_material, fifty_moves, repetition or none
  "promotions": { "q": 1, "r": 0, "b": 0, "n": 1 },
  "castled": { "white": 9, "black": null },     // the ply of the castling move, counted in this game
  "captures": 27,
  "material": { "20": 0, "40": 1, "end": 5 },   // white's less black's, P 1, N B 3, R 5, Q 9; after plies 20 & 40 when reached

  // when not ok, instead of the facts:
  "error": { "ply": 7, "reason": "Ke3: [IO Error] san_parser :: No matching moves found for SAN: e3" },

  // with --verbose:
  "moves": ["e2e4", "e7e5"],      // UCI, castling as the king's move
  "fen": "..."                    // the final position, or where it stopped
}
```

`ended` is what the final position says by the rules, in that order: a game that ended by resignation, adjudication,
time or a crash is `none`. A game starts from its `FEN` tag when it has one, `SetUp` or not.

Tags are read as python-chess reads them: a value runs from the first quote to the last `"]` of its line and is kept
as written, `\"` included, and a line that isn't a tag is skipped. Movetext: move numbers, NAGs, `!?` suffixes,
variations and `%` lines are skipped, `{}` and `;` comments are read past.

## Checked against python-chess

`test/indexer_smoke.py` writes games as PGN, reads them back with phant-indexer and python-chess, and compares every
game's moves, final position and entry. Its games are `test/pgn/*.pgn` (OpenBench's and fastchess's output and edge
cases) and random games played by python-chess, which promote, castle and capture en passant far more often than
engines do. `test/smoke.sh` runs it with 2000 random games. The entries' format is pinned by
`test/src/indexer/game_index_test.cpp`.

## Not built yet

- bz2 and gzip input.
- Several files at once on several threads (`--jobs`).
- The producer's mapping, OpenBench's player names to the match's engines and the `Subject` & `Opponent` tags, applied
  here and written into the PGN, so nyayo's worker doesn't edit PGN text.

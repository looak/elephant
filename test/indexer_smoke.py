#!/usr/bin/env python3
"""phant-indexer against python-chess, a full circle: games are written as PGN, phant-indexer reads them back, and
every game's entry has to agree with what python-chess reads from the same PGN.

    python test/indexer_smoke.py --indexer build/Release/bin/phant-indexer
    python test/indexer_smoke.py --indexer ... --random 5000 --seed 7 test/pgn/*.pgn
    python test/indexer_smoke.py --indexer ... --keep logs/indexer_random.pgn     keep the generated games
    python test/indexer_smoke.py --indexer ... --random 0 --games ../uwanja/runs --limit 5000   real games, sampled

The games are the PGN files given, test/pgn/*.pgn when none are, the *.pgn files under each --games directory (up
to --limit files, picked at random by the seed), and --random games played by python-chess at
random from the start or from a random position (a FEN tag), with comments, NAGs and variations sprinkled in, so the
tokenizer sees all of PGN and the SAN parser every kind of move: random play promotes, underpromotes, castles and
captures en passant far more often than engines do. Each game is compared on: whether it read and where it stopped,
the moves in UCI notation, the final position, and every fact of the entry (result, plies, how it ended, promotions,
castling, captures, en passant captures, checks, the fewest pieces, material). Each file is read compressed too, bz2,
gzip, zstd in one frame and zstd a frame every 64 games (as a batch may be written), on standard input, and has to
give the same entries. Prints both readers' speed. Exits 1 on a disagreement,
2 when it couldn't run.

Deliberately not python-chess's: a Variant other than standard chess (Chess960 included) is an error before the moves,
since Elephant doesn't play it.

Needs python-chess (pip install chess), which is what nyayo reads games with today, and zstd: Python 3.14's
compression.zstd or the zstandard package (pip install zstandard), which nyayo writes its batches with.
"""
from __future__ import annotations

import argparse
import bz2
import gzip
import io
import json
import logging
import random
import subprocess
import sys
import tempfile
import time
from pathlib import Path

try:
    import chess
    import chess.pgn
except ImportError:
    print("indexer_smoke: needs python-chess, pip install chess", file=sys.stderr)
    sys.exit(2)
try:
    from compression import zstd  # Python 3.14

    def zstd_compress(data: bytes) -> bytes:
        return zstd.compress(data)
except ImportError:
    try:
        import zstandard

        def zstd_compress(data: bytes) -> bytes:
            return zstandard.ZstdCompressor().compress(data)
    except ImportError:
        print("indexer_smoke: needs zstd, Python 3.14 or pip install zstandard", file=sys.stderr)
        sys.exit(2)
logging.getLogger("chess.pgn").setLevel(logging.CRITICAL)  # an illegal move is a fact here, not an error

ROOT = Path(__file__).resolve().parent
FORMAT = 1
STANDARD = {"", "standard", "chess", "classical", "normal", "illegal", "from position"}  # python-chess's chess.Board
MATERIAL_PLIES = (20, 40)
VALUES = {chess.PAWN: 1, chess.KNIGHT: 3, chess.BISHOP: 3, chess.ROOK: 5, chess.QUEEN: 9}
PROMOTIONS = {chess.QUEEN: "q", chess.ROOK: "r", chess.BISHOP: "b", chess.KNIGHT: "n"}
ROSTER = {"Event": "?", "Site": "?", "Date": "????.??.??", "Round": "?", "White": "?", "Black": "?", "Result": "*"}


def material(board: chess.Board) -> int:
    return sum(VALUES.get(p.piece_type, 0) * (1 if p.color == chess.WHITE else -1) for p in board.piece_map().values())


def ending(board: chess.Board) -> str:
    """How the final position ended the game by the rules, in the order phant-indexer checks them."""
    if board.is_checkmate():
        return "checkmate"
    if board.is_stalemate():
        return "stalemate"
    if board.is_insufficient_material():
        return "insufficient_material"
    if board.halfmove_clock >= 100:
        return "fifty_moves"
    if board.is_repetition(3):
        return "repetition"
    return None


def fen(board: chess.Board) -> str:
    # phant-indexer keeps an en passant square a pawn could capture on, pins ignored: python-chess's "xfen".
    return board.fen(en_passant="xfen")


def expected(game: chess.pgn.Game) -> dict:
    """The entry python-chess reads from a game, the fields phant-indexer's verbose entry has."""
    entry = {"ok": not game.errors, "tags": dict(game.headers), "result": game.headers.get("Result", "*"), "moves": []}
    if game.headers.get("Variant", "").lower() not in STANDARD:
        entry.update(ok=False, plies=0, error_ply=0)  # phant-indexer plays standard chess only
        del entry["moves"]
        return entry
    board = game.board()
    promotions = dict.fromkeys("qrbn", 0)
    castled = {"white": None, "black": None}
    captures = ep_captures = checks = plies = 0
    min_pieces = len(board.piece_map())
    mat = {}
    for move in game.mainline_moves():
        plies += 1
        if move:  # a null move is falsy, and python-chess calls one taking on a1 a capture
            if board.is_capture(move):
                captures += 1
            if board.is_en_passant(move):
                ep_captures += 1
            if move.promotion:
                promotions[PROMOTIONS[move.promotion]] += 1
            if board.is_castling(move):
                castled["white" if board.turn == chess.WHITE else "black"] = plies
        entry["moves"].append(move.uci())
        board.push(move)
        if move and board.is_check():
            checks += 1
        min_pieces = min(min_pieces, len(board.piece_map()))
        if plies in MATERIAL_PLIES:
            mat[str(plies)] = material(board)
    entry["plies"] = plies
    entry["fen"] = fen(board)
    if entry["ok"]:
        mat["end"] = material(board)
        entry.update(ended=ending(board), promotions=promotions, castled=castled, captures=captures,
                     ep_captures=ep_captures, checks=checks, min_pieces=min_pieces, material=mat)
    else:
        entry["error_ply"] = plies + 1
    return entry


def compare(want: dict, got: dict) -> list[str]:
    diffs = []
    # python-chess fills in the seven tag roster, phant-indexer reports the tags written.
    got = {**got, "tags": {**ROSTER, **got.get("tags", {})}}
    # and writes the movetext's result into a Result tag of *, phant-indexer reports the tag as written.
    if got["tags"]["Result"] == "*" and want["tags"].get("Result") == want["result"]:
        got["tags"]["Result"] = want["result"]
    if got.get("format") != FORMAT:
        diffs.append(f"format {got.get('format')!r}, expected {FORMAT!r}")
    if not got.get("reader"):
        diffs.append("no reader version")
    for key in ("ok", "tags", "result", "plies", "moves", "fen", "ended", "promotions", "castled", "captures",
                "ep_captures", "checks", "min_pieces", "material"):
        if key in want and want[key] != got.get(key):
            if key == "moves":
                at = next((i for i, (a, b) in enumerate(zip(want[key], got.get(key) or [])) if a != b), None)
                diffs.append(f"moves differ at ply {at + 1 if at is not None else 'end'}: "
                             f"python-chess {want[key][at:at + 3] if at is not None else len(want[key])}, "
                             f"phant-indexer {(got.get(key) or [])[at:at + 3] if at is not None else len(got.get(key) or [])}")
            else:
                diffs.append(f"{key}: python-chess {want[key]!r}, phant-indexer {got.get(key)!r}")
    if not want["ok"] and got.get("error", {}).get("ply") != want["error_ply"]:
        diffs.append(f"error ply: python-chess {want['error_ply']}, phant-indexer {got.get('error')}")
    return diffs


def random_game(rng: random.Random) -> chess.pgn.Game:
    """A game played at random, from the start or from a random position, decorated the ways PGN allows."""
    board = chess.Board()
    if rng.random() < 0.3:
        for _ in range(rng.randrange(4, 60)):
            moves = list(board.legal_moves)
            if not moves or board.is_game_over():
                break
            board.push(rng.choice(moves))
        board = chess.Board(board.fen())
        if board.is_game_over():
            board = chess.Board()
    game = chess.pgn.Game()
    if board.fen() != chess.STARTING_FEN:
        game.setup(board)
    game.headers["White"] = rng.choice(["Elephant 1.3.0", 'Elephant "herd" \\ dev', "Weiss-dev"])
    node = game
    for _ in range(rng.randrange(1, 400)):
        if board.is_game_over(claim_draw=False):
            break
        moves = list(board.legal_moves)
        # promote & castle more than random play would, those are the moves SAN gets wrong.
        special = [m for m in moves if m.promotion or board.is_castling(m) or board.is_en_passant(m)]
        move = rng.choice(special) if special and rng.random() < 0.5 else rng.choice(moves)
        if rng.random() < 0.05 and len(moves) > 1:
            node.add_variation(rng.choice([m for m in moves if m != move])).comment = "a sideline"
        node = node.add_main_variation(move)
        if rng.random() < 0.2:
            node.comment = rng.choice(["+0.31/12 0.120s", "+1.31 14/40 159 377044", "book", "see [1]; (and) $2"])
        if rng.random() < 0.05:
            node.nags.add(rng.choice([1, 2, 3, 4, 5, 6]))
        board.push(move)
    outcome = board.outcome(claim_draw=False)
    game.headers["Result"] = outcome.result() if outcome else rng.choice(["1-0", "0-1", "1/2-1/2", "*"])
    return game


def run_indexer(indexer: str, path: Path, stdin: bytes | None = None) -> tuple[list[dict], float]:
    """phant-indexer's entries for a file, or for bytes on standard input."""
    start = time.perf_counter()
    args = [indexer, "--verbose"] if stdin is not None else [indexer, "--verbose", str(path)]
    done = subprocess.run(args, input=stdin, capture_output=True, check=False)
    elapsed = time.perf_counter() - start
    if done.returncode != 0:
        print(f"indexer_smoke: phant-indexer exited {done.returncode} on {path}: {done.stderr.decode(errors='replace')}",
              file=sys.stderr)
        sys.exit(2)
    return [json.loads(line) for line in done.stdout.decode("utf-8").splitlines()], elapsed


def compressed_agree(indexer: str, path: Path, entries: list[dict]) -> list[str]:
    """The same entries from the file bz2'd, gzipped and zstd'd on standard input, but for the source."""
    plain = [{k: v for k, v in entry.items() if k != "source"} for entry in entries]
    data = path.read_bytes()
    diffs = []
    # zstd frames of 64 games each, split where the games start.
    starts = [entry["offset"] for entry in entries[::64]][1:]
    pieces = [data[a:b] for a, b in zip([0, *starts], [*starts, len(data)])]
    framed = b"".join(zstd_compress(piece) for piece in pieces)
    for name, packed in (("bz2", bz2.compress(data)), ("gzip", gzip.compress(data)), ("zstd", zstd_compress(data)),
                         (f"zstd in {len(pieces)} frames", framed)):
        got, _ = run_indexer(indexer, path, stdin=packed)
        if got != plain:
            diffs.append(f"{name} on standard input: {len(got)} entries, not the plain file's {len(plain)}"
                         if len(got) != len(plain) else f"{name} on standard input: entries differ")
    return diffs


def read_games(path: Path) -> tuple[list[chess.pgn.Game], float]:
    start = time.perf_counter()
    games = []
    with open(path, encoding="utf-8-sig") as stream:
        while (game := chess.pgn.read_game(stream)) is not None:
            games.append(game)
    return games, time.perf_counter() - start


def check_file(indexer: str, path: Path, label: str) -> tuple[int, int, float, float]:
    """Compares one PGN file, returns (games, disagreements, phant-indexer seconds, python-chess seconds)."""
    entries, indexer_time = run_indexer(indexer, path)
    games, python_time = read_games(path)
    bad = 0
    for diff in compressed_agree(indexer, path, entries):
        print(f"FAIL {label}: {diff}")
        bad += 1
    if len(entries) != len(games):
        print(f"FAIL {label}: phant-indexer read {len(entries)} games, python-chess {len(games)}")
        return len(games), max(1, abs(len(entries) - len(games))), indexer_time, python_time
    for number, (game, entry) in enumerate(zip(games, entries), start=1):
        diffs = compare(expected(game), entry)
        if diffs:
            bad += 1
            if bad <= 10:
                print(f"FAIL {label} game {number} (byte {entry.get('offset')}):")
                for diff in diffs:
                    print(f"       {diff}")
    return len(games), bad, indexer_time, python_time


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--indexer", required=True, help="the phant-indexer binary")
    parser.add_argument("--random", type=int, default=2000, help="random games to play, default 2000")
    parser.add_argument("--seed", type=int, default=None, help="for the random games, default a new one each run")
    parser.add_argument("--keep", type=Path, help="write the random games here, to rerun a failure")
    parser.add_argument("--games", type=Path, action="append", default=[],
                        help="a directory of real games, *.pgn under it; can be given more than once")
    parser.add_argument("--limit", type=int, default=5000, help="at most this many files from --games, default 5000")
    parser.add_argument("pgn", nargs="*", type=Path, help="PGN files, default test/pgn/*.pgn")
    args = parser.parse_args()

    indexer = Path(args.indexer).resolve()
    if not indexer.is_file():
        print(f"indexer_smoke: no phant-indexer at {args.indexer}", file=sys.stderr)
        return 2
    files = args.pgn or ([] if args.games else sorted((ROOT / "pgn").glob("*.pgn")))
    seed = args.seed if args.seed is not None else random.randrange(1 << 30)
    total_games = total_bad = 0
    indexer_time = python_time = 0.0

    with tempfile.TemporaryDirectory() as scratch:
        if args.games:
            # real games, the files picked at random by the seed and written as one.
            texts = []
            for path in sorted(p for directory in args.games for p in directory.rglob("*.pgn")):
                texts.append(path.read_text(encoding="utf-8-sig", errors="replace").strip() + "\n\n")
            random.Random(seed).shuffle(texts)
            sample = Path(scratch) / "games.pgn"
            sample.write_text("".join(texts[:args.limit]), encoding="utf-8", newline="\n")
            files = [*files, sample]
        if args.random:
            rng = random.Random(seed)
            generated = args.keep or Path(scratch) / "random.pgn"
            with open(generated, "w", encoding="utf-8", newline="\n") as out:
                for _ in range(args.random):
                    print(random_game(rng), file=out, end="\n\n")
            files = [*files, generated]

        for path in files:
            label = (f"random (seed {seed})" if args.random and path == files[-1]
                     else f"real games (seed {seed})" if args.games and path.name == "games.pgn" else path.name)
            games, bad, it, pt = check_file(str(indexer), path, label)
            total_games, total_bad = total_games + games, total_bad + bad
            indexer_time, python_time = indexer_time + it, python_time + pt
            print(f"{'PASS' if not bad else 'FAIL'} {label}: {games} games, {bad} disagree")

    rate = lambda seconds: f"{total_games / seconds:,.0f} games/s" if seconds else "-"
    print(f"phant-indexer {rate(indexer_time)} (verbose, process start included), python-chess {rate(python_time)}")
    if total_bad:
        print(f"indexer_smoke: {total_bad} of {total_games} games disagree, seed {seed}")
        return 1
    print(f"indexer_smoke: all {total_games} games agree, seed {seed}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

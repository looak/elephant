#!/usr/bin/env python3
"""Runs EPD suites against a UCI engine, many positions at once, and writes every position's result, see test/RESULTS.md.

    python test/epd_runner.py --engine build/Release/bin/ElephantGambit --tier smoke --out checks.jsonl
    python test/epd_runner.py --engine ... --tier extended --check Extended_STS --jobs 8
    python test/epd_runner.py --engine ... --tier smoke --nodes 300000        an experiment, compared only with experiments

Any UCI engine works. Every position is its own game: ucinewgame, position fen, go with the check's limit. With one
thread at a fixed node count that's deterministic, the result doesn't depend on which engine process searched it or
what it searched before, so the positions are spread over --jobs engine processes. A check with T threads runs
jobs / T engines.

The engine is asked "buildinfo" (non standard, see below) for what it was built from. An engine that doesn't know the
command ignores it, as UCI says, and the build is the "id name" and the binary's hash.

    > buildinfo
    > isready
    < buildinfo commit 9db97f8c...         one "buildinfo <key> <value>" line per entry, any keys
    < buildinfo compiler gcc 13.3.0        compiler, config, assertions, exceptions & bits decide what's comparable
    < readyok

A position passes when the move played is one of bm and none of am. A scored check (STS) also totals the points of the
c0 list, "f5=10, Be5+=2". The tiers live in test/epd_tiers.json. Each check is written as one JSON line, test/results.py
record turns the lines into a run. Exits 1 when a check falls below its minimum, 2 when it couldn't run.

Standard library only: SAN is matched against the engine's move without a move generator, the engine's move is legal
and a SAN move names one legal move, so the piece, the target and any disambiguation decide it.
"""
from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import os
import queue
import re
import subprocess
import sys
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CASE_TIMEOUT = 600.0   # seconds a single position may take before the engine is considered hung


# --- EPD ---

def split_operations(text: str) -> list[tuple[str, list[str]]]:
    """'bm Qg6; id "WAC.001";' -> [('bm', ['Qg6']), ('id', ['WAC.001'])], quoted operands may hold ; and spaces."""
    operations, tokens, current, quoted = [], [], "", False
    for ch in text + ";":
        if quoted:
            if ch == '"':
                tokens.append(current)
                current, quoted = "", False
            else:
                current += ch
        elif ch == '"':
            quoted = True
        elif ch in " \t;":
            if current:
                tokens.append(current)
                current = ""
            if ch == ";" and tokens:
                operations.append((tokens[0], tokens[1:]))
                tokens = []
        else:
            current += ch
    return operations


def parse_epd_line(line: str) -> dict | None:
    fields = line.strip().split(None, 4)
    if len(fields) < 4 or line.lstrip().startswith("#"):
        return None
    ops = dict(split_operations(fields[4]) if len(fields) > 4 else [])
    case = {"fen": " ".join(fields[:4]) + f" {ops.get('hmvc', ['0'])[0]} {ops.get('fmvn', ['1'])[0]}",
            "id": (ops.get("id") or [""])[0], "expected": ops.get("bm", []), "avoid": ops.get("am", []),
            "scores": []}
    # c0 is a score list "f5=10, Be5+=2" in STS, a plain comment elsewhere.
    comment = " ".join(ops.get("c0", []))
    items = [item.strip() for item in comment.split(",")]
    if comment and all(re.fullmatch(r"\S+=\d+", item) for item in items):
        case["scores"] = [(move, int(points)) for move, points in (item.rsplit("=", 1) for item in items)]
    return case


def load_cases(epd_dir: Path, patterns: list[str], per_file: int) -> tuple[list[str], dict[str, str], list[dict]]:
    """the files a check runs (globs expanded, natural order), their content hashes, and the positions."""
    names = []
    for pattern in patterns:
        matched = sorted((p.name for p in epd_dir.iterdir() if fnmatch.fnmatch(p.name, pattern)),
                         key=lambda name: [int(part) if part.isdigit() else part for part in re.split(r"(\d+)", name)])
        names += [name for name in matched if name not in names]
    hashes, cases = {}, []
    for name in names:
        data = (epd_dir / name).read_bytes()
        hashes[name] = hashlib.sha256(data).hexdigest()
        file_cases = [c for c in (parse_epd_line(line) for line in data.decode("utf-8", "replace").splitlines()) if c]
        for index, case in enumerate(file_cases[:per_file] if per_file else file_cases):
            case["file"] = name
            case["id"] = case["id"] or f"{name}.{index + 1}"
            cases.append(case)
    return names, hashes, cases


# --- moves: SAN against the engine's coordinate move ---

SAN = re.compile(r"^([NBRQK])?([a-h])?([1-8])?x?([a-h][1-8])(?:=?([NBRQ]))?$")


def piece_at(fen: str, square: str) -> str:
    """the piece letter on square, white upper case, "" for empty."""
    rows = fen.split()[0].split("/")
    row = rows[8 - int(square[1])]
    file = ord(square[0]) - ord("a")
    col = 0
    for ch in row:
        if ch.isdigit():
            col += int(ch)
        else:
            if col == file:
                return ch
            col += 1
    return ""


def san_matches(fen: str, san: str, played: str) -> bool | None:
    """True when the coordinate move played is the SAN move, None when san can't be read."""
    if re.fullmatch(r"[a-h][1-8][a-h][1-8][qrbn]?", san):
        return san == played
    san = san.rstrip("+#!?")
    if not re.fullmatch(r"[a-h][1-8][a-h][1-8][qrbn]?", played or ""):
        return False
    source, target, promotion = played[:2], played[2:4], played[4:].upper()
    mover = piece_at(fen, source)
    if san in ("O-O", "0-0", "O-O-O", "0-0-0"):
        long = san.count("O") + san.count("0") == 3
        return mover.upper() == "K" and source[0] == "e" and target[0] == ("c" if long else "g")
    m = SAN.match(san)
    if m is None:
        return None
    piece, from_file, from_rank, to, promote = m.groups()
    if mover.upper() != (piece or "P") or target != to or (promote or "") != promotion:
        return False
    return (from_file is None or source[0] == from_file) and (from_rank is None or source[1] == from_rank)


# --- engine ---

class EngineError(Exception):
    pass


class Engine:
    """one engine process, talked to over UCI. A reader thread queues its output lines."""

    def __init__(self, command: list[str], options: dict[str, str]):
        try:
            self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                            stderr=subprocess.DEVNULL, text=True, bufsize=1,
                                            cwd=Path(command[0]).parent)
        except OSError as error:
            raise EngineError(f"can't start {command[0]}: {error}") from error
        self.lines: queue.Queue[str | None] = queue.Queue()
        threading.Thread(target=self._read, daemon=True).start()
        self.send("uci")
        self.name = ""
        for line in self.until("uciok", 30):
            if "id name " in line:   # after a prompt in some engines, "> id name ..."
                self.name = line[line.index("id name ") + 8:].strip()
        for name, value in options.items():
            self.send(f"setoption name {name} value {value}")
        self.send("buildinfo")
        self.send("isready")
        self.build: dict[str, str] = {}
        for line in self.until("readyok", 60):
            parts = line.split(None, 2)
            if len(parts) == 3 and parts[0] == "buildinfo":
                self.build[parts[1]] = parts[2]

    def _read(self) -> None:
        for line in self.process.stdout:
            self.lines.put(line.rstrip("\r\n"))
        self.lines.put(None)

    def send(self, text: str) -> None:
        try:
            self.process.stdin.write(text + "\n")
            self.process.stdin.flush()
        except OSError as error:
            raise EngineError(f"the engine went away: {error}") from error

    def until(self, token: str, timeout: float) -> list[str]:
        """the lines up to the one starting with token, that one included."""
        deadline, lines = time.monotonic() + timeout, []
        while True:
            try:
                line = self.lines.get(timeout=max(0.0, deadline - time.monotonic()))
            except queue.Empty:
                raise EngineError(f"no {token} within {timeout:.0f} s") from None
            if line is None:
                raise EngineError(f"the engine exited, exit code {self.process.wait()}")
            lines.append(line)
            if line == token or line.startswith(token + " "):
                return lines

    def search(self, fen: str, go: str) -> dict:
        self.send("ucinewgame")
        self.send("isready")
        self.until("readyok", 60)
        self.send(f"position fen {fen}")
        start = time.monotonic()
        self.send(f"go {go}")
        lines = self.until("bestmove", CASE_TIMEOUT)
        result = {"got": lines[-1].split()[1] if len(lines[-1].split()) > 1 else "", "score": 0, "mate": False,
                  "nodes": 0, "time_ms": round((time.monotonic() - start) * 1000), "pv": ""}
        for line in lines:
            tokens = line.split()
            if not tokens or tokens[0] != "info" or "string" in tokens[:2]:
                continue
            if "score" in tokens:
                i = tokens.index("score")
                result["mate"] = tokens[i + 1] == "mate"
                result["score"] = int(tokens[i + 2])
            if "nodes" in tokens:
                result["nodes"] = int(tokens[tokens.index("nodes") + 1])
            if "pv" in tokens:
                result["pv"] = " ".join(tokens[tokens.index("pv") + 1:])
        return result

    def close(self) -> None:
        try:
            self.send("quit")
            self.process.wait(timeout=5)
        except (EngineError, subprocess.TimeoutExpired):
            self.process.kill()


# --- a check ---

def go_command(limit: dict) -> str:
    kind, value = next(iter(limit.items()))
    return f"{kind} {value}"


def score_case(case: dict, result: dict, scored: bool) -> dict:
    fen, played = case["fen"], result["got"]
    best = [san_matches(fen, san, played) for san in case["expected"]]
    avoid = [san_matches(fen, san, played) for san in case["avoid"]]
    error = None in best or None in avoid or not played
    solved = (not best or any(best)) and not any(avoid)
    record = {"id": case["id"], "file": case["file"], "fen": fen, "status": "error" if error else
              "pass" if solved else "fail", "expected": case["expected"], "avoid": case["avoid"]}
    record.update(result)
    if scored:
        if case["scores"]:
            # an unreadable c0 alternative scores nothing, STS lists a few illegal moves.
            record["points"] = max((points for san, points in case["scores"] if san_matches(fen, san, played)),
                                   default=0)
            record["max_points"] = max(points for _, points in case["scores"])
        else:
            record["points"], record["max_points"] = (10 if solved else 0), 10
    return record


def run_check(check: dict, args: argparse.Namespace, engine_command: list[str]) -> tuple[dict, str]:
    """runs one check, returns its JSON record and the engine's verdict: pass, warn, fail or none."""
    threads = int(check.get("threads", 1))
    hash_mb = int(check.get("hash", 64))
    limit = dict(check["limit"])
    experiment = False
    for kind in ("nodes", "depth", "movetime"):
        if getattr(args, kind):
            limit, experiment = {kind: getattr(args, kind)}, True
    if args.threads:
        threads, experiment = args.threads, True
    if args.hash:
        hash_mb, experiment = args.hash, True

    names, hashes, cases = load_cases(Path(args.epd_dir), check["files"], int(check.get("per_file", 0)))
    if not cases:
        raise EngineError(f"{check['name']}: no positions in {', '.join(check['files'])} under {args.epd_dir}")
    workers = max(1, min(args.jobs // threads, len(cases)))
    options = {"Threads": str(threads), "Hash": str(hash_mb)}
    go = go_command(limit)
    scored = bool(check.get("scored"))

    pending: queue.Queue[int] = queue.Queue()
    for index in range(len(cases)):
        pending.put(index)
    records: list[dict | None] = [None] * len(cases)
    builds: list[tuple[str, dict]] = []
    done = [0]
    lock = threading.Lock()

    def work() -> None:
        engine = None
        while True:
            try:
                index = pending.get_nowait()
            except queue.Empty:
                break
            case = cases[index]
            try:
                if engine is None:
                    engine = Engine(engine_command, options)
                    with lock:
                        builds.append((engine.name, engine.build))
                record = score_case(case, engine.search(case["fen"], go), scored)
            except EngineError as error:
                record = {"id": case["id"], "file": case["file"], "fen": case["fen"], "status": "error",
                          "expected": case["expected"], "avoid": case["avoid"], "got": "", "error": str(error)}
                if engine is not None:
                    engine.process.kill()
                engine = None   # a fresh one for the next position
            with lock:
                records[index] = record
                done[0] += 1
                if not args.quiet:
                    print(f"\r  {check['name']}: {done[0]}/{len(cases)}", end="", flush=True)
        if engine is not None:
            engine.close()

    start = time.monotonic()
    pool = [threading.Thread(target=work) for _ in range(workers)]
    for thread in pool:
        thread.start()
    for thread in pool:
        thread.join()
    seconds = time.monotonic() - start
    if not args.quiet:
        print("\r", end="")

    solved = sum(1 for r in records if r["status"] == "pass")
    errors = [r for r in records if r["status"] == "error"]
    metrics = {"solved": {"value": solved}, "total": {"value": len(records)}}
    if scored:
        metrics["points"] = {"value": sum(r.get("points", 0) for r in records)}
        metrics["max_points"] = {"value": sum(r.get("max_points", 10) for r in records)}
    metrics["duration"] = {"value": round(seconds, 1), "unit": "s"}

    # the levels don't apply to an experiment, the settings differ from the ones they were measured with.
    repeatable = bool(check.get("repeatable", True))
    baseline = 0 if experiment else int(check.get("baseline", 0))
    minimum = 0 if experiment else int(check.get("minimum", 0))
    value = metrics["points"]["value"] if scored else solved
    if value < minimum:
        verdict = "fail"
    elif baseline == 0 or not repeatable:
        verdict = "pass" if minimum > 0 else "none"
    else:
        verdict = "warn" if value < baseline else "pass"

    name, build = builds[0] if builds else ("", {})
    record = {
        "id": check["id"], "version": 1, "name": check["name"],
        "status": "error" if len(errors) == len(records) else "pass",
        "deterministic": threads == 1 and "movetime" not in limit,
        "params": {"limit": {"type": next(iter(limit)), "value": next(iter(limit.values()))}, "threads": threads,
                   "hash_mb": hash_mb, "per_file": int(check.get("per_file", 0)), "files": names,
                   "file_sha256": hashes, "experiment": experiment, "workers": workers},
        "metrics": metrics,
        "engine_verdict": {"baseline": baseline, "minimum": minimum, "repeatable": repeatable, "result": verdict},
        "build": build_record(name, build, engine_command[0]),
        "cases": records,
    }

    summary = f"{check['name']}: {solved}/{len(records)} solved ({100.0 * solved / len(records):.1f}%)"
    if scored:
        summary += f", {metrics['points']['value']}/{metrics['max_points']['value']} points"
    summary += f", baseline {baseline}, minimum {minimum}, {go}, threads {threads}, hash {hash_mb}, " \
               f"{workers} engines, {seconds:.0f} s"
    print(summary)
    if verdict == "fail":
        print(f"FAIL {check['name']}: {value} is below the minimum {minimum}")
    elif repeatable and baseline and value < baseline:
        print(f"WARNING {check['name']}: {value} is {baseline - value} below the baseline {baseline}, minimum {minimum}")
    elif repeatable and baseline and value > baseline:
        print(f"NOTE {check['name']}: {value} is {value - baseline} above the baseline {baseline}, "
              "raise it if the change is meant to stay")
    for error in errors[:5]:
        print(f"ERROR {check['name']}: {error['id']} {error.get('error', 'a bm/am that cannot be read')}")
    return record, verdict


_binary_hashes: dict[str, str] = {}


def build_record(name: str, build: dict, binary: str) -> dict:
    """the engine's buildinfo with the five keys that decide comparability typed, plus the id name & binary hash."""
    record: dict = {"engine": name}
    for key, value in build.items():
        if key in ("assertions", "exceptions", "dirty"):
            record[key] = value == "true"
        elif key == "bits":
            record[key] = int(value) if value.isdigit() else value
        else:
            record[key] = value
    if binary not in _binary_hashes:
        try:
            _binary_hashes[binary] = hashlib.sha256(Path(binary).read_bytes()).hexdigest()
        except OSError:
            _binary_hashes[binary] = ""
    record["binary_sha256"] = _binary_hashes[binary]
    return record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--engine", required=True, help="the UCI engine binary, arguments after it in quotes")
    parser.add_argument("--tier", required=True, help="a tier in the config, smoke or extended")
    parser.add_argument("--config", default=str(ROOT / "test" / "epd_tiers.json"))
    parser.add_argument("--epd-dir", default=str(ROOT / "res"), help="where the EPD files are")
    parser.add_argument("--check", action="append", help="only these checks, by name (repeatable)")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1, help="engine threads in use at once")
    parser.add_argument("--out", help="append each check as a JSON line here, for test/results.py record")
    parser.add_argument("--quiet", action="store_true", help="no progress counter, also off when not on a terminal")
    experiment = parser.add_argument_group("experiments, override every check's settings")
    experiment.add_argument("--nodes", type=int)
    experiment.add_argument("--depth", type=int)
    experiment.add_argument("--movetime", type=int)
    experiment.add_argument("--threads", type=int)
    experiment.add_argument("--hash", type=int)
    args = parser.parse_args()
    args.quiet = args.quiet or not sys.stdout.isatty()

    tiers = json.loads(Path(args.config).read_text(encoding="utf-8"))
    if args.tier not in tiers or args.tier.startswith("_"):
        print(f"epd_runner: no tier {args.tier} in {args.config}", file=sys.stderr)
        return 2
    checks = [c for c in tiers[args.tier] if not args.check or c["name"] in args.check]
    if not checks:
        print(f"epd_runner: no checks named {', '.join(args.check)} in {args.tier}", file=sys.stderr)
        return 2

    engine_command = args.engine.split() if not Path(args.engine).exists() else [args.engine]
    engine_command[0] = str(Path(engine_command[0]).resolve())
    failed = False
    for check in checks:
        try:
            record, verdict = run_check(check, args, engine_command)
        except EngineError as error:
            print(f"epd_runner: {error}", file=sys.stderr)
            return 2
        failed |= verdict == "fail"
        if args.out:
            with open(args.out, "a", encoding="utf-8") as out:
                out.write(json.dumps(record, separators=(",", ":")) + "\n")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

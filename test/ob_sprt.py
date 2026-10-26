#!/usr/bin/env python3
"""Queues an SPRT on OpenBench from the command line, the create test page's form filled with Elephant's presets.

    python test/ob_sprt.py eval-pieces-mopup-edge eval-pieces              STC, [0, 5]
    python test/ob_sprt.py eval-pieces-mopup-edge eval-pieces --nonreg     STC, [-3, 1]
    python test/ob_sprt.py eval-pieces-pairs eval-pieces --ltc             LTC
    python test/ob_sprt.py dev base --tc 8+0.08 --bounds -1,4 --dry-run    the form only, nothing sent
    python test/ob_sprt.py dev base --nonreg --book Endgames.epd --win-adj None
                                            endgame starts, played out to mate or a draw

dev and base are branches or commits, pushed to GitHub: OpenBench reads them from there, a local one it can't see.
A branch is checked against its remote, and the bench each side's commit message carries is printed; OpenBench takes
the bench from the message.

The presets are what the tests so far were run at: STC 8+0 Threads=1 Hash=8, LTC 40+0 Threads=1 Hash=64, book
8moves_v3.epd, the engine's adjudication and expected nps from OpenBench's Engines/elephant.json. (Its presets say
8.00.08 and 40.00.4, which OpenBench reads as 8+0 and 40+0, the time control is spelled out here.)

OpenBench: git config openbench.url, openbench.username & openbench.password, or OPENBENCH_URL, OPENBENCH_USERNAME &
OPENBENCH_PASSWORD. The url defaults to http://172.16.100.11:8000. The account has to be enabled to create tests, and
an approver's tests start at once.

Standard library only.
"""
from __future__ import annotations

import argparse
import html
import http.cookiejar
import os
import re
import subprocess
import sys
import urllib.parse
import urllib.request

REPO = "https://github.com/looak/elephant"
DEFAULT_URL = "http://172.16.100.11:8000"

PRESETS = {
    "stc": {"time_control": "8.0+0.00", "options": "Threads=1 Hash=8", "workload_size": 32},
    "ltc": {"time_control": "40.0+0.00", "options": "Threads=1 Hash=64", "workload_size": 8},
}
BOUNDS = {"gain": "[0.00, 5.00]", "nonreg": "[-3.00, 1.00]"}

# the rest of the form, as Engines/elephant.json and the create test page have them.
FORM_DEFAULTS = {
    "dev_engine": "elephant",
    "base_engine": "elephant",
    "dev_repo": REPO,
    "base_repo": REPO,
    "dev_network": "",
    "base_network": "",
    "book_name": "8moves_v3.epd",
    "upload_pgns": "COMPACT",
    "test_mode": "SPRT",
    "test_confidence": "[0.05, 0.05]",
    "test_max_games": "",
    "priority": "0",
    "throughput": "1000",
    "syzygy_wdl": "DISABLED",
    "syzygy_adj": "OPTIONAL",
    "win_adj": "movecount=3 score=400",
    "draw_adj": "movenumber=40 movecount=8 score=10",
    "scale_method": "DEV",
    "scale_nps": "1198000",
}


def git(*args: str) -> str:
    return subprocess.run(["git", *args], capture_output=True, text=True, check=True).stdout.strip()


def setting(name: str) -> str | None:
    """openbench.<name> from git config, or OPENBENCH_<NAME>."""
    try:
        if value := git("config", f"openbench.{name}"):
            return value
    except subprocess.CalledProcessError:
        pass
    return os.environ.get(f"OPENBENCH_{name.upper()}")


def resolve(ref: str) -> tuple[str, str | None]:
    """ref's commit as GitHub has it, and the bench its message carries. Exits when GitHub doesn't have it."""
    if re.fullmatch(r"[0-9a-fA-F]{7,40}", ref):
        try:
            sha = git("rev-parse", "--verify", f"{ref}^{{commit}}")
        except subprocess.CalledProcessError:
            sys.exit(f"{ref}: no such commit here, fetch first")
        if not git("branch", "-r", "--contains", sha):
            sys.exit(f"{ref}: not on any remote branch, push it first")
    else:
        try:
            sha = git("rev-parse", "--verify", f"origin/{ref}")
        except subprocess.CalledProcessError:
            sys.exit(f"{ref}: no origin/{ref}, push it first (or git fetch)")
        try:
            local = git("rev-parse", "--verify", ref)
            if local != sha:
                print(f"  note: local {ref} is {local[:7]}, origin has {sha[:7]}; OpenBench tests origin's")
        except subprocess.CalledProcessError:
            pass
    bench = re.search(r"^bench (\d+)", git("log", "-1", "--format=%B", sha), re.MULTILINE)
    return sha, bench.group(1) if bench else None


def form(args: argparse.Namespace) -> dict[str, str]:
    preset = PRESETS["ltc" if args.ltc else "stc"]
    time_control = args.tc or preset["time_control"]
    options = args.options or preset["options"]
    bounds = f"[{args.bounds}]" if args.bounds else BOUNDS["nonreg" if args.nonreg else "gain"]
    data = dict(FORM_DEFAULTS)
    data.update({
        "dev_branch": args.dev,
        "base_branch": args.base,
        "dev_options": options,
        "base_options": options,
        "dev_time_control": time_control,
        "base_time_control": time_control,
        "workload_size": str(preset["workload_size"]),
        "test_bounds": bounds,
        "action": "CREATE_TEST",
    })
    if args.book:
        data["book_name"] = args.book
    if args.win_adj:
        data["win_adj"] = args.win_adj
    if args.draw_adj:
        data["draw_adj"] = args.draw_adj
    if args.priority is not None:
        data["priority"] = str(args.priority)
    return data


def message(page: str, kind: str) -> str | None:
    """The error or warning OpenBench left on the page it redirected to."""
    found = re.search(rf'<div class="{kind}-message">\s*<pre>(.*?)</pre>', page, re.DOTALL)
    return html.unescape(found.group(1)).strip() if found else None


def newest_test(opener: urllib.request.OpenerDirector, url: str) -> int | None:
    page = opener.open(f"{url}/index/").read().decode("utf-8", "replace")
    ids = [int(found) for found in re.findall(r"/test/(\d+)/", page)]
    return max(ids) if ids else None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("dev", help="branch or commit under test")
    parser.add_argument("base", help="branch or commit it's tested against")
    parser.add_argument("--ltc", action="store_true", help="the long time control preset, 40+0 Hash=64")
    parser.add_argument("--nonreg", action="store_true", help="non-regression bounds [-3, 1], for fixes & reworks")
    parser.add_argument("--bounds", help="Elo bounds, e.g. -1,4; overrides --nonreg")
    parser.add_argument("--tc", help="time control, e.g. 8+0.08; overrides the preset's")
    parser.add_argument("--options", help="UCI options for both, e.g. 'Threads=1 Hash=16'; overrides the preset's")
    parser.add_argument("--book", help="opening book as OpenBench names it, e.g. Endgames.epd; 8moves_v3.epd by default")
    parser.add_argument("--win-adj", help="win adjudication, e.g. 'movecount=3 score=400', or None to play games out")
    parser.add_argument("--draw-adj", help="draw adjudication, e.g. 'movenumber=40 movecount=8 score=10', or None")
    parser.add_argument("--priority", type=int, help="OpenBench's priority, 0 by default; the worker plays only the "
                        "highest priority's tests, so -1 waits for the others and 1 goes first")
    parser.add_argument("--dry-run", action="store_true", help="print the form, send nothing")
    args = parser.parse_args()

    git("fetch", "--quiet", "origin")
    for side, ref in (("dev", args.dev), ("base", args.base)):
        sha, bench = resolve(ref)
        print(f"  {side:4} {ref} = {sha[:7]}, bench {bench or 'MISSING from the message, OpenBench will refuse it'}")

    data = form(args)
    print(f"  test {data['dev_time_control']} {data['dev_options']} {data['test_bounds']} {data['book_name']}, "
          f"win adj {data['win_adj']}, draw adj {data['draw_adj']}, priority {data['priority']}")
    if args.dry_run:
        for key, value in data.items():
            print(f"    {key} = {value}")
        return 0

    url = (setting("url") or DEFAULT_URL).rstrip("/")
    username, password = setting("username"), setting("password")
    if not username or not password:
        sys.exit("no OpenBench login: git config openbench.username & openbench.password, or OPENBENCH_USERNAME & "
                 "OPENBENCH_PASSWORD")
    data.update({"username": username, "password": password})

    # the session cookie carries the login, and the message the redirect's page shows.
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
    response = opener.open(f"{url}/scripts/", urllib.parse.urlencode(data).encode())
    page = response.read().decode("utf-8", "replace")
    landed = urllib.parse.urlparse(response.geturl()).path

    if error := message(page, "error"):
        print(f"  refused: {error}")
        return 1
    if landed != "/index/":
        print(f"  refused: OpenBench sent us to {landed}")
        return 1
    if warning := message(page, "warning"):
        print(f"  warning: {warning}")
    test_id = newest_test(opener, url)
    print(f"  queued: {url}/test/{test_id}/" if test_id else "  queued, its id not found on the index")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Queues an SPRT on nyayo from the command line: the create test page's preset, with what the command line changes.

    python test/ob_sprt.py eval-pieces-mopup-edge eval-pieces              STC, [0, 5]
    python test/ob_sprt.py eval-pieces-mopup-edge eval-pieces --nonreg     STC, [-3, 1]
    python test/ob_sprt.py eval-pieces-pairs eval-pieces --ltc             LTC
    python test/ob_sprt.py dev base --tc 8+0.08 --bounds -1,4 --dry-run    the request only, nothing created
    python test/ob_sprt.py dev base --nonreg --book Endgames.epd --win-adj None
                                            endgame starts, played out to mate or a draw

dev and base are branches or commits, pushed to GitHub: nyayo reads them from there, a local one it can't see. A
branch is checked against its remote, and the bench each side's commit message carries is printed; nyayo takes the
bench from the message.

The time control, options, book, adjudication, expected nps and the rest are nyayo's: its Elephant engine config and the
preset named STC (the default) or LTC (--ltc). Only what a flag changes is sent, by the field names of nyayo's create
page (dev_tc, bounds, book, ...). --dry-run fetches the presets and prints the JSON it would send.

nyayo's server is `git config nyayo.url` or NYAYO_URL, http://172.16.100.11:8001 by default, and `nyayo.token` or
NYAYO_TOKEN is the token it is called with. The owner makes one on bobby: `manage.py api_token create <name>`.
When nyayo refuses the test its errors and warnings are printed and the exit status is 1; a warning only is cleared
by sending the same command with --anyway.

Standard library only.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.request

ENGINE = "Elephant"
DEFAULT_URL = "http://172.16.100.11:8001"
NONREG_BOUNDS = "[-3.00, 1.00]"


def git(*args: str) -> str:
    return subprocess.run(["git", *args], capture_output=True, text=True, check=True).stdout.strip()


def setting(name: str) -> str | None:
    """nyayo.<name> from git config, or NYAYO_<NAME>."""
    try:
        if value := git("config", f"nyayo.{name}"):
            return value
    except (subprocess.CalledProcessError, OSError):
        pass
    return os.environ.get(f"NYAYO_{name.upper()}")


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
                print(f"  note: local {ref} is {local[:7]}, origin has {sha[:7]}; nyayo tests origin's")
        except subprocess.CalledProcessError:
            pass
    bench = re.search(r"^bench (\d+)", git("log", "-1", "--format=%B", sha), re.MULTILINE)
    return sha, bench.group(1) if bench else None


def bounds(text: str) -> str:
    """'-1,4' as the form spells bounds, '[-1.00, 4.00]'."""
    try:
        low, high = (float(part) for part in text.strip("[] ").split(","))
    except ValueError:
        sys.exit(f"--bounds {text}: expected two numbers, like -1,4")
    return f"[{low:.2f}, {high:.2f}]"


def body(args: argparse.Namespace) -> dict[str, object]:
    """The request: engine, preset, the two branches, and the fields a flag overrides."""
    data: dict[str, object] = {
        "engine": ENGINE,
        "preset": "LTC" if args.ltc else "STC",
        "dev_branch": args.dev,
        "base_branch": args.base,
    }
    if args.nonreg:
        # the page labels a test by its kind, so --nonreg says it is a non-regression test, with its bounds
        # unless --bounds names others.
        data["test_kind"] = "regression"
        data["bounds"] = NONREG_BOUNDS
    if args.bounds:
        data["bounds"] = bounds(args.bounds)
    if args.tc:
        data["dev_tc"] = data["base_tc"] = args.tc
    if args.options:
        data["dev_options"] = data["base_options"] = args.options
    if args.book:
        data["book"] = args.book
    if args.win_adj:
        data["win_adj"] = args.win_adj
    if args.draw_adj:
        data["draw_adj"] = args.draw_adj
    if args.priority is not None:
        data["priority"] = str(args.priority)
    if args.anyway:
        data["anyway"] = True
    return data


def call(url: str, token: str | None, data: dict | None = None) -> tuple[int, dict]:
    """The status and JSON body of nyayo's answer; a 4xx is an answer, an unreachable server exits."""
    headers = {"Accept": "application/json"}
    if token:
        headers["Authorization"] = f"Bearer {token}"
    payload = None
    if data is not None:
        payload = json.dumps(data).encode()
        headers["Content-Type"] = "application/json"
    request = urllib.request.Request(url, payload, headers)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            status, raw = response.status, response.read()
    except urllib.error.HTTPError as error:
        status, raw = error.code, error.read()
    except (urllib.error.URLError, OSError) as error:
        sys.exit(f"{url}: {error}")
    try:
        return status, json.loads(raw or b"{}")
    except ValueError:
        sys.exit(f"{url}: HTTP {status}, not JSON: {raw[:200]!r}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("dev", help="branch or commit under test")
    parser.add_argument("base", help="branch or commit it's tested against")
    parser.add_argument("--ltc", action="store_true", help="nyayo's LTC preset, 40+0.4 Hash=64; STC is the default")
    parser.add_argument("--nonreg", action="store_true", help="non-regression bounds [-3, 1], for fixes & reworks")
    parser.add_argument("--bounds", help="Elo bounds, e.g. -1,4; overrides --nonreg")
    parser.add_argument("--tc", help="time control, e.g. 8+0.08; overrides the preset's")
    parser.add_argument("--options", help="UCI options for both, e.g. 'Threads=1 Hash=16'; overrides the preset's")
    parser.add_argument("--book", help="opening book as nyayo names it, e.g. Endgames.epd; the preset's by default")
    parser.add_argument("--win-adj", help="win adjudication, e.g. 'movecount=3 score=400', or None to play games out")
    parser.add_argument("--draw-adj", help="draw adjudication, e.g. 'movenumber=40 movecount=8 score=10', or None")
    parser.add_argument("--priority", type=int, help="the priority, 0 by default; the worker plays only the "
                        "highest priority's tests, so -1 waits for the others and 1 goes first")
    parser.add_argument("--anyway", action="store_true", help="create the test although nyayo warns about it")
    parser.add_argument("--dry-run", action="store_true", help="fetch the presets, print the request, create nothing")
    # argparse takes "-1,4" for an option, so "--bounds -1,4" is read as "--bounds=-1,4".
    argv = list(sys.argv[1:] if argv is None else argv)
    for i, word in enumerate(argv[:-1]):
        if word == "--bounds":
            argv[i:i + 2] = [f"--bounds={argv[i + 1]}"]
            break
    args = parser.parse_args(argv)

    git("fetch", "--quiet", "origin")
    for side, ref in (("dev", args.dev), ("base", args.base)):
        sha, bench = resolve(ref)
        print(f"  {side:4} {ref} = {sha[:7]}, bench {bench or 'MISSING from the message, nyayo will refuse it'}")

    data = body(args)
    url = (setting("url") or DEFAULT_URL).rstrip("/")
    token = setting("token")

    if args.dry_run:
        status, found = call(f"{url}/api/tests/presets?engine={ENGINE}", token)
        if status != 200:
            print(f"  presets: HTTP {status} {json.dumps(found)}")
            return 1
        preset = found.get("presets", {}).get(data["preset"])
        if preset is None:
            print(f"  no preset {data['preset']} for {ENGINE}, nyayo has {', '.join(found.get('presets', {})) or 'none'}")
            return 1
        print(f"  preset {data['preset']}: {json.dumps(preset)}")
        print(f"  POST {url}/api/tests")
        print(json.dumps(data, indent=2))
        return 0

    if not token:
        sys.exit("no nyayo token: git config nyayo.token, or NYAYO_TOKEN. The owner makes one on bobby with "
                 "`manage.py api_token create <name>`")

    status, answer = call(f"{url}/api/tests", token, data)
    if status == 201:
        for warning in answer.get("warnings", []):
            print(f"  warning: {warning}")
        print(f"  queued: {answer.get('url') or answer.get('id')}")
        return 0
    for error in answer.get("errors", []):
        print(f"  error: {error}")
    for warning in answer.get("warnings", []):
        print(f"  warning: {warning}")
    if status == 401:
        print("  nyayo didn't accept the token: check nyayo.token, or have the owner make a new one")
    elif status == 400 and not answer.get("errors") and answer.get("warnings"):
        print("  nothing was created; --anyway sends it again, past the warnings")
    elif not answer:
        print(f"  refused: HTTP {status}")
    return 1


if __name__ == "__main__":
    sys.exit(main())

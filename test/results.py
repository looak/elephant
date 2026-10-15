#!/usr/bin/env python3
"""Smoke & extended EPD results: records runs, keeps their history and syncs them to nyayo, see test/RESULTS.md.

    python test/results.py record --checks FILE --kind smoke    the checks test/epd_runner.py wrote, as a run
    python test/results.py sync                                 update run states, upload what nyayo hasn't got
    python test/results.py compare [RUN] [--against RUN]        positions that changed, a run vs its ancestor's
    python test/results.py compare --previous                   ... vs the previous run on the same branch
    python test/results.py case WAC.297                         one position through history
    python test/results.py list                                 the recorded runs

A run is wip, committed or published. A run on a dirty working tree is wip: it records the tree its files would commit
as, is compared with history but isn't part of it. Once a commit has exactly that tree the run is that commit's,
committed, and once the commit is on a remote branch it's published, only published runs are history for good. sync,
run by test/smoke.sh and the post-commit hook, moves runs along and uploads them. Runs are compared along the commit
graph, a branch with the commit it descends from, so branches waiting on SPRT don't mix.

nyayo: git config nyayo.url & nyayo.token, or NYAYO_URL & NYAYO_TOKEN. Without them runs stay local. CI runs aren't
uploaded, nyayo pulls them from the workflow's artifacts.

Standard library only, the store is logs/results/ (per machine, ignored by git) unless --store says otherwise.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import uuid
from datetime import datetime, timezone
from pathlib import Path

SCHEMA_VERSION = 1
ROOT = Path(__file__).resolve().parent.parent
ANCESTORS = 300   # commits of ancestry a run carries (topo order), enough for nyayo to place it without a clone


# --- git ---

def git(*args: str, env: dict | None = None, check: bool = True) -> str:
    result = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, env=env)
    if check and result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)}: {result.stderr.strip()}")
    return result.stdout.strip()


def working_tree() -> str:
    """The tree id the working copy would commit as with everything added, untracked files included."""
    with tempfile.TemporaryDirectory() as tmp:
        index = Path(tmp) / "index"
        git_index = Path(git("rev-parse", "--git-path", "index"))
        if not git_index.is_absolute():
            git_index = ROOT / git_index
        if git_index.exists():
            shutil.copy(git_index, index)
        env = {**os.environ, "GIT_INDEX_FILE": str(index)}
        git("add", "-A", env=env)
        return git("write-tree", env=env)


def ancestors(commit: str, limit: int = 5000) -> list[str]:
    """commit and its ancestors, nearest first."""
    out = git("rev-list", "--topo-order", f"--max-count={limit}", commit, check=False)
    return out.split() if out else []


def commit_for_tree(tree: str) -> str | None:
    """the newest commit, on any branch, whose tree is tree."""
    out = git("log", "--all", "--format=%H %T", "-n", "2000", check=False)
    for line in out.splitlines():
        commit, commit_tree = line.split()
        if commit_tree == tree:
            return commit
    return None


def is_published(commit: str) -> bool:
    """on a remote branch, as far as the last fetch knows."""
    return bool(git("branch", "-r", "--contains", commit, check=False))


def subject(commit: str) -> str:
    return git("log", "-1", "--format=%s", commit, check=False)


# --- store ---

def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def store_dirs(store: Path) -> tuple[Path, Path]:
    history, wip = store / "history", store / "wip"
    history.mkdir(parents=True, exist_ok=True)
    wip.mkdir(parents=True, exist_ok=True)
    legacy = store / "inflight"   # before wip had its name
    if legacy.is_dir():
        for path in legacy.glob("*.json"):
            path.replace(wip / path.name)
        legacy.rmdir()
    return history, wip


def upgrade(run: dict) -> bool:
    """fills in what runs recorded before ids, ancestry & the wip/committed/published states lack."""
    r = run["run"]
    changed = False
    if "id" not in r:
        r["id"], changed = str(uuid.uuid4()), True
    if "source" not in r:
        r["source"], changed = ("ci" if r.get("ci") else "dev"), True
    if r.get("state") == "in_flight":
        r["state"], changed = "wip", True
    if "promoted_at" in r:   # the first client's name for committed_at
        r.setdefault("committed_at", r["promoted_at"])
        del r["promoted_at"]
        changed = True
    if "ancestors" not in r or len(r["ancestors"]) == 100:   # recorded before the ancestry went to 300
        r["ancestors"], changed = ancestors(r["commit"] or r["base_commit"], ANCESTORS), True
    return changed


def load_runs(store: Path) -> list[tuple[Path, dict]]:
    runs = []
    for folder in store_dirs(store):
        for path in folder.glob("*.json"):
            try:
                run = json.loads(path.read_text(encoding="utf-8"))
            except (OSError, json.JSONDecodeError) as error:
                print(f"results: skipping {path}: {error}", file=sys.stderr)
                continue
            if upgrade(run):
                write_run(path, run)
            runs.append((path, run))
    runs.sort(key=lambda item: item[1]["run"]["recorded_at"])
    return runs


def write_run(path: Path, run: dict) -> None:
    path.write_text(json.dumps(run, separators=(",", ":")), encoding="utf-8")


def run_label(run: dict) -> str:
    r = run["run"]
    where = r["commit"][:10] if r["commit"] else f"on {r['base_commit'][:10]}"
    return f"{r['kind']} {r['state']} {where} ({r['branch']}) {r['recorded_at']}"


# --- record ---

def sha256(path: Path) -> str | None:
    try:
        return hashlib.sha256(path.read_bytes()).hexdigest()
    except OSError:
        return None


def host() -> dict:
    cpu = platform.processor() or platform.machine()
    try:
        if sys.platform.startswith("linux"):
            for line in Path("/proc/cpuinfo").read_text().splitlines():
                if line.startswith("model name"):
                    cpu = line.split(":", 1)[1].strip()
                    break
    except OSError:
        pass
    return {"name": platform.node(), "os": f"{platform.system()} {platform.release()}", "cpu": cpu,
            "logical_cores": os.cpu_count()}


def record(args: argparse.Namespace) -> int:
    checks = []
    for line in Path(args.checks).read_text(encoding="utf-8").splitlines():
        if line.strip():
            checks.append(json.loads(line))
    if not checks:
        print(f"results: no checks in {args.checks}", file=sys.stderr)
        return 1

    # the suite files' content, an edited EPD reusing old ids isn't comparable with its history. The runner hashes
    # the files it read, older checks didn't.
    for check in checks:
        params = check.get("params", {})
        if "file_sha256" not in params:
            params["file_sha256"] = {name: sha256(ROOT / "res" / name) for name in params.get("files", [])}

    builds = {json.dumps(check.get("build"), sort_keys=True) for check in checks}
    build = json.loads(builds.pop()) if len(builds) == 1 else None
    head = git("rev-parse", "HEAD")
    # the engine says what it was built from (buildinfo), a binary from another commit isn't this commit's result.
    built_from = (build or {}).get("commit")
    if built_from and built_from != head:
        print(f"results: the engine was built from {built_from[:10]}, HEAD is {head[:10]}; rebuild it, nothing "
              "recorded", file=sys.stderr)
        return 1
    tree = working_tree()
    clean = tree == git("rev-parse", "HEAD^{tree}")
    if (build or {}).get("dirty") is False and not clean:
        print("results: the engine was built from a clean tree, the working tree has changed since; rebuild it, "
              "nothing recorded", file=sys.stderr)
        return 1
    ci = bool(os.environ.get("GITHUB_ACTIONS"))
    state = "wip" if not clean else "published" if ci or is_published(head) else "committed"
    now = now_iso()
    run = {
        "schema_version": SCHEMA_VERSION,
        "run": {
            "id": str(uuid.uuid4()),
            "kind": args.kind,
            "source": "ci" if ci else "dev",
            "trigger": args.trigger,
            "request_id": None,
            "state": state,
            "commit": head if clean else None,
            "base_commit": head,
            "tree": tree,
            "branch": os.environ.get("GITHUB_HEAD_REF") or os.environ.get("GITHUB_REF_NAME")
                      or git("rev-parse", "--abbrev-ref", "HEAD"),
            "ancestors": ancestors(head, ANCESTORS),
            "version": (build or {}).get("version") or (ROOT / "version.txt").read_text(encoding="utf-8").strip(),
            "started_at": args.started or now,
            "finished_at": now,
            "recorded_at": now,
            "host": host(),
            "build": build,
        },
        "checks": checks,
    }
    if ci and os.environ.get("GITHUB_RUN_ID"):
        server = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
        run["run"]["ci"] = {"workflow_run_id": int(os.environ["GITHUB_RUN_ID"]),
                            "url": f"{server}/{os.environ.get('GITHUB_REPOSITORY', '')}/actions/runs/{os.environ['GITHUB_RUN_ID']}"}

    history, wip = store_dirs(Path(args.store))
    stamp = now.replace(":", "").replace("-", "")[:15]
    path = (history / f"{head[:12]}-{args.kind}-{stamp}.json") if clean \
        else (wip / f"{tree[:12]}-{args.kind}-{stamp}.json")
    write_run(path, run)
    if not args.quiet:
        print(f"results: recorded {run_label(run)}")
    return 0


# --- sync: states & nyayo ---

def advance_states(store: Path, quiet: bool) -> None:
    """wip runs whose tree got committed become that commit's, committed runs on a remote branch become published."""
    history, _ = store_dirs(store)
    for path, run in load_runs(store):
        r = run["run"]
        if r["state"] == "wip":
            commit = commit_for_tree(r["tree"])
            if commit is None:
                continue
            r["state"], r["commit"], r["committed_at"] = "committed", commit, now_iso()
            r["ancestors"] = ancestors(commit, ANCESTORS)
            target = history / f"{commit[:12]}-{r['kind']}-{path.stem.rsplit('-', 1)[-1]}.json"
            write_run(target, run)
            path.unlink()
            print(f"results: {run_label(run)}, {subject(commit)}")
            path = target
        if r["state"] == "committed" and is_published(r["commit"]):
            r["state"], r["published_at"] = "published", now_iso()
            write_run(path, run)
            if not quiet:
                print(f"results: {run_label(run)}")


def nyayo_config() -> tuple[str | None, str | None]:
    url = os.environ.get("NYAYO_URL") or git("config", "--get", "nyayo.url", check=False) or None
    token = os.environ.get("NYAYO_TOKEN") or git("config", "--get", "nyayo.token", check=False) or None
    return (url.rstrip("/") if url else None), token


def upload(url: str, token: str | None, run: dict) -> None:
    """PUT /api/runs/{id}, the whole run each time: idempotent, a state change is the same call again."""
    body = gzip.compress(json.dumps(run, separators=(",", ":")).encode("utf-8"))
    request = urllib.request.Request(f"{url}/api/runs/{run['run']['id']}", data=body, method="PUT",
                                     headers={"Content-Type": "application/json", "Content-Encoding": "gzip"})
    if token:
        request.add_header("Authorization", f"Bearer {token}")
    with urllib.request.urlopen(request, timeout=15) as response:
        if response.status not in (200, 201, 204):
            raise RuntimeError(f"HTTP {response.status}")


def sync(args: argparse.Namespace) -> int:
    store = Path(args.store)
    advance_states(store, args.quiet)

    url, token = nyayo_config()
    if url is None:
        if not args.quiet:
            print("results: nyayo isn't configured (git config nyayo.url), runs stay local")
        return 0

    # what nyayo has: per run id the hash of the version sent, and the versions it rejected with why. A rejected
    # version isn't sent again, the run is once it changes (promoted, published).
    sent_path = store / "uploaded.json"
    try:
        state = json.loads(sent_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        state = {}
    if "sent" not in state:   # the first format, run id -> hash
        state = {"sent": state, "rejected": {}}
    sent, rejected = state["sent"], state["rejected"]

    uploaded, refused = 0, []
    for _, run in load_runs(store):
        digest = hashlib.sha256(json.dumps(run, sort_keys=True).encode("utf-8")).hexdigest()
        run_id = run["run"]["id"]
        if sent.get(run_id) == digest or rejected.get(run_id, {}).get("hash") == digest:
            continue
        try:
            upload(url, token, run)
        except urllib.error.HTTPError as error:
            reason = error.read().decode("utf-8", "replace").strip()[:300] or error.reason
            if error.code in (401, 403) or error.code >= 500:
                # the token or the server, every run would fail the same way: stop, retry next time.
                print(f"results: nyayo refused the upload, HTTP {error.code} {reason}; sync retries later")
                break
            # this run: 400 invalid, 409 a change nyayo doesn't allow under its id. Note it and carry on.
            rejected[run_id] = {"hash": digest, "status": error.code, "reason": reason, "at": now_iso()}
            refused.append(f"{run_label(run)}: HTTP {error.code} {reason}")
            continue
        except (OSError, urllib.error.URLError, RuntimeError) as error:
            print(f"results: nyayo unreachable, {error}; sync retries later")
            break
        sent[run_id] = digest
        rejected.pop(run_id, None)
        uploaded += 1
    sent_path.write_text(json.dumps(state, indent=1), encoding="utf-8")
    if uploaded or not args.quiet:
        print(f"results: {uploaded} run(s) uploaded to {url}")
    for line in refused:
        print(f"results: nyayo rejected {line}")
    return 0


# --- compare ---

def check_key(check: dict, build: dict | None) -> str:
    """checks with equal keys searched the same way: the same limit, threads, hash & build. per_file only picks
    positions, every position is searched on its own, so it isn't part of the key."""
    params = check.get("params", {})
    key = {"id": check["id"], "version": check.get("version"), "limit": params.get("limit"),
           "threads": params.get("threads"), "hash_mb": params.get("hash_mb"),
           "experiment": params.get("experiment", False),
           "build": {k: (build or {}).get(k) for k in ("compiler", "config", "assertions", "exceptions", "bits")}}
    return json.dumps(key, sort_keys=True)


def comparable_checks(run: dict) -> dict[str, dict]:
    return {check_key(c, c.get("build") or run["run"].get("build")): c for c in run["checks"]}


def case_key(check: dict, case: dict) -> tuple[str, str | None]:
    return case["id"], check.get("params", {}).get("file_sha256", {}).get(case.get("file"))


def find_run(runs: list[tuple[Path, dict]], name: str | None) -> dict | None:
    if name is None:
        return runs[-1][1] if runs else None
    for path, run in reversed(runs):
        r = run["run"]
        if name in (path.name, path.stem, r["id"]) or (r["commit"] or "").startswith(name) \
                or r["tree"].startswith(name) or r["id"].startswith(name):
            return run
    return None


def find_baseline(runs: list[tuple[Path, dict]], target: dict) -> dict | None:
    """the run on the nearest ancestor of the target with a comparable check: at the same commit published before
    committed, then the newest. wip runs are never a baseline."""
    t = target["run"]
    lineage = ancestors(t["base_commit"])
    if t["commit"]:
        lineage = lineage[1:]   # a committed run compares with what came before it
    position = {commit: i for i, commit in enumerate(lineage)}
    keys = set(comparable_checks(target))
    best, best_position = None, None
    for _, run in runs:
        r = run["run"]
        if r["state"] == "wip" or r["commit"] not in position or run is target:
            continue
        if not keys & set(comparable_checks(run)):
            continue
        rank = (position[r["commit"]], r["state"] != "published", _negated(r["recorded_at"]))
        if best is None or rank < best_position:
            best, best_position = run, rank
    return best


def _negated(timestamp: str) -> tuple:
    """sorts ISO timestamps newest first."""
    return tuple(-ord(ch) for ch in timestamp)


def find_previous(runs: list[tuple[Path, dict]], target: dict) -> dict | None:
    """the run before the target on its branch, wip runs included, with a comparable check."""
    keys = set(comparable_checks(target))
    previous = None
    for _, run in runs:
        if run is target:
            break
        if run["run"]["branch"] == target["run"]["branch"] and keys & set(comparable_checks(run)):
            previous = run
    return previous


def describe_case(case: dict) -> str:
    text = f"{case['id']:<36} {case['status']:<5} got {case.get('got', ''):<6}"
    if "points" in case:
        text += f" {case['points']:>2}/{case.get('max_points', 10)}"
    return text


def compare_runs(target: dict, baseline: dict, brief: bool) -> list[str]:
    lines = [f"compare {run_label(target)}", f"   with {run_label(baseline)}"
             + (f", {subject(baseline['run']['commit'])}" if baseline["run"]["commit"] else "")]
    base_checks = comparable_checks(baseline)
    target_checks = comparable_checks(target)
    same_tree = target["run"]["tree"] == baseline["run"]["tree"]
    metric = lambda c, m: c.get("metrics", {}).get(m, {}).get("value")
    for key, check in target_checks.items():
        base = base_checks.get(key)
        name = check.get("name", check["id"])
        harness = "" if check.get("status", "pass") == "pass" else f" [harness {check['status']}]"
        if base is None:
            same_id = [c for c in base_checks.values() if c["id"] == check["id"]]
            why = "the same suite ran with other settings or another build" if same_id else "a new check"
            lines.append(f"  {name}: no comparable run, {why}{harness}")
            continue
        totals = f"solved {metric(base, 'solved')} -> {metric(check, 'solved')}"
        if metric(check, "points") is not None:
            totals += f", points {metric(base, 'points')} -> {metric(check, 'points')}"
        if not check.get("deterministic", True):
            lines.append(f"  {name}: {totals} (multi threaded, totals only){harness}")
            continue
        changed_files = sorted(f for f, sha in check.get("params", {}).get("file_sha256", {}).items()
                               if base.get("params", {}).get("file_sha256", {}).get(f) not in (None, sha))
        base_cases = {case_key(base, c): c for c in base.get("cases", [])}
        target_keys = {case_key(check, c) for c in check.get("cases", [])}
        newly_failing, newly_passing, points_changed, moves_changed = [], [], [], []
        compared = new = 0
        for case in check.get("cases", []):
            before = base_cases.get(case_key(check, case))
            if before is None:
                new += 1
                continue
            compared += 1
            if before["status"] == "pass" and case["status"] != "pass":
                newly_failing.append((before, case))
            elif before["status"] != "pass" and case["status"] == "pass":
                newly_passing.append((before, case))
            elif case.get("points", 0) != before.get("points", 0):
                points_changed.append((before, case))
            elif case.get("got") != before.get("got"):
                moves_changed.append((before, case))
        missing = sum(1 for k in base_cases if k not in target_keys)
        summary = f"  {name}: {totals}, {compared} positions compared: {len(newly_failing)} newly failing, " \
                  f"{len(newly_passing)} newly passing"
        if points_changed:
            summary += f", {len(points_changed)} with other points"
        if moves_changed:
            summary += f", {len(moves_changed)} other moves, same result"
        if changed_files:
            summary += f"; suite file changed ({', '.join(changed_files)}), its positions aren't comparable"
        else:
            if new:
                summary += f"; {new} new positions"
            if missing:
                summary += f"; {missing} positions missing"
        lines.append(summary + harness)
        if same_tree and (newly_failing or newly_passing or points_changed or moves_changed):
            lines.append("    NOT DETERMINISTIC: the same tree and build searched differently")
        if brief:
            continue
        for label, changes in (("newly failing", newly_failing), ("newly passing", newly_passing),
                               ("other points", points_changed)):
            for before, case in changes:
                lines.append(f"    {label:<14} {describe_case(case)}   was {before.get('got', '')}"
                             + (f" {before['points']}/{before.get('max_points', 10)}" if "points" in before else ""))
    for key, base in base_checks.items():
        if key not in target_checks:
            lines.append(f"  {base.get('name', base['id'])}: missing, the baseline ran it")
    return lines


def compare(args: argparse.Namespace) -> int:
    runs = load_runs(Path(args.store))
    target = find_run(runs, args.run)
    if target is None:
        print("results: no runs recorded yet" if args.run is None else f"results: no run matches {args.run}")
        return 1
    if args.against:
        baseline = find_run(runs, args.against)
    elif args.previous:
        baseline = find_previous(runs, target)
    else:
        baseline = find_baseline(runs, target)
    if baseline is None:
        where = "earlier on its branch" if args.previous else "on an ancestor commit"
        print(f"results: nothing to compare {run_label(target)} with, no comparable run {where}")
        return 0
    print("\n".join(compare_runs(target, baseline, args.brief)))
    return 0


# --- case ---

def case_history(args: argparse.Namespace) -> int:
    runs = load_runs(Path(args.store))
    order = {commit: i for i, commit in enumerate(reversed(ancestors(args.at or "HEAD")))}
    rows = []
    for _, run in runs:
        r = run["run"]
        anchor = r["commit"] or r["base_commit"]
        if anchor not in order:
            continue
        for check in run["checks"]:
            if args.check and check["id"] != args.check:
                continue
            for case in check.get("cases", []):
                if case["id"] == args.case:
                    rows.append((order[anchor], r["recorded_at"], run, check, case))
    if not rows:
        print(f"results: {args.case} isn't in any run on the history of {args.at or 'HEAD'}")
        return 1
    rows.sort(key=lambda row: (row[0], row[1]))
    # "last passed" only counts runs that searched the latest one's way, the same check, settings & build.
    latest_key = check_key(rows[-1][3], rows[-1][3].get("build") or rows[-1][2]["run"].get("build"))
    last_pass = None
    for _, _, run, check, case in rows:
        r = run["run"]
        where = r["commit"][:10] if r["commit"] else "wip"
        limit = check.get("params", {}).get("limit", {})
        print(f"  {where:<10} {r['state']:<9} {r['kind']:<8} {check.get('name', check['id']):<28} {limit.get('type')} "
              f"{limit.get('value'):<9} {describe_case(case)}  {r['branch']}")
        if case["status"] == "pass" and r["commit"] and check_key(check, check.get("build") or r.get("build")) == latest_key:
            last_pass = r["commit"]
    latest_check, latest = rows[-1][3], rows[-1][4]
    if latest["status"] != "pass":
        name = latest_check.get("name", latest_check["id"])
        print(f"  {args.case} last passed in {name} at {last_pass[:10]}, {subject(last_pass)}" if last_pass
              else f"  {args.case} hasn't passed in {name} in any recorded run")
    return 0


def list_runs(args: argparse.Namespace) -> int:
    for path, run in load_runs(Path(args.store)):
        summary = ", ".join(
            f"{c.get('name', c['id'])} {c.get('metrics', {}).get('points', c.get('metrics', {}).get('solved', {})).get('value')}"
            for c in run["checks"])
        print(f"  {run_label(run)}: {summary}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--store", default=str(ROOT / "logs" / "results"), help="where runs are kept")
    commands = parser.add_subparsers(dest="command", required=True)

    p = commands.add_parser("record", help="record the checks test/epd_runner.py wrote as a run")
    p.add_argument("--checks", required=True, help="the runner's --out file")
    p.add_argument("--kind", required=True, choices=["smoke", "extended"])
    p.add_argument("--trigger", default="local", help="local, push, schedule, request")
    p.add_argument("--started", help="when the run started, ISO 8601")
    p.add_argument("--quiet", action="store_true")
    p.set_defaults(func=record)

    p = commands.add_parser("sync", help="move runs to committed & published, upload what nyayo hasn't got")
    p.add_argument("--quiet", action="store_true", help="only say what changed")
    p.set_defaults(func=sync)

    p = commands.add_parser("compare", help="positions that changed between two runs")
    p.add_argument("run", nargs="?", help="a run: id, file name, commit or tree prefix (default the newest)")
    p.add_argument("--against", help="the run to compare with (default the nearest ancestor's)")
    p.add_argument("--previous", action="store_true", help="compare with the previous run on the same branch")
    p.add_argument("--brief", action="store_true", help="totals only, no positions")
    p.set_defaults(func=compare)

    p = commands.add_parser("case", help="one position through history")
    p.add_argument("case", help="the EPD id, e.g. WAC.297")
    p.add_argument("--check", help="only this check, e.g. epd.wac_new")
    p.add_argument("--at", help="the history of this commit or branch (default HEAD)")
    p.set_defaults(func=case_history)

    p = commands.add_parser("list", help="the recorded runs")
    p.set_defaults(func=list_runs)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

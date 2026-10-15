# EPD results records

Every position the EPD suites search is recorded, so a change can be judged position by position against history:
which positions a branch newly fails or solves, and since which commit a position has been failing. Elo and speed
stay with OpenBench, these records are about behaviour. The results format belongs to
[nyayo](https://github.com/looak/nyayo), which collects the runs; this file describes what Elephant produces.

## The runner

`test/epd_runner.py` plays the EPD suites with any UCI engine and writes every position's result. Each position is its
own game, `ucinewgame`, `position fen`, `go` with the check's limit, so the positions are spread over one engine
process per core (`--jobs`, a check with 4 threads gets jobs / 4 engines). The smoke tier takes seconds.

```sh
python test/epd_runner.py --engine <binary> --tier smoke --out checks.jsonl     # the tiers in test/epd_tiers.json
python test/epd_runner.py --engine <binary> --tier extended --check Extended_STS
python test/epd_runner.py --engine <binary> --tier smoke --nodes 300000         # an experiment, see below
python test/epd_runner.py --engine <binary> --tier extended --leave 4 --low-priority   # keep using the machine
```

The tiers, checks, limits and levels are data in `test/epd_tiers.json`: a new suite is an EPD file in `res/` and an
entry there, no rebuild. A check below its `baseline` warns, below its `minimum` it fails (exit code 1). The runner is
standard library only and knows nothing about Elephant, SAN is matched against the engine's move without a move
generator.

**Regression suite.** `res/regression.epd` holds positions the engine once got wrong, every one must pass
(`Smoke_Regression`, minimum = the count, depth 23 since the cases are depth bound, REG.003 is found at 22). A fixed bug that shows up as a
wrong move, from an OpenBench loss, a bwawa game or an issue, adds a line: `bm` or `am` in SAN, the next `REG.NNN` id
and a `c1` note saying where it came from. Raise `baseline` & `minimum` with it, and the depth if a case needs more.

**buildinfo.** The runner asks the engine what it was built from with a non standard command. Engines ignore commands
they don't know, as UCI says, so whatever arrives before `readyok` is the answer and an engine without it still works:

```
> buildinfo
> isready
< buildinfo version 1.0.0
< buildinfo commit 9db97f8c3e...        the full hash, written on every build
< buildinfo dirty false                 the working tree had changes when it was built
< buildinfo branch main                 left out on a detached HEAD
< buildinfo compiler gcc 13.3.0
< buildinfo config release
< buildinfo assertions false
< buildinfo exceptions true
< buildinfo bits 64
< readyok
```

Any keys go, `compiler`, `config`, `assertions`, `exceptions` & `bits` decide which runs are comparable. Without
buildinfo the build is the engine's `id name` and the binary's SHA-256. `results.py record` refuses checks from a
binary built from another commit than HEAD, or from a clean tree when the working tree has changed since.

## Workflow

```sh
test/smoke.sh                                  # runs the smoke tier, records the run, compares it, syncs
python test/results.py compare                 # the newest run against the nearest ancestor commit's run
python test/results.py compare --previous      # against the previous run on the same branch, wip included
python test/results.py compare --against <run> # against a chosen run: id, file name, commit or tree prefix
python test/results.py case WAC.297            # one position through the history of HEAD
python test/results.py list                    # every recorded run
python test/results.py sync                    # move runs along wip -> committed -> published, upload to nyayo
```

Runs are kept in `logs/results/` (per machine, ignored by git): `wip/` for runs on uncommitted changes, `history/`
for runs that belong to a commit.

**States.** A run is `wip`, `committed` or `published`.

- `wip`: recorded on a dirty working tree. The run records the tree its files would commit as (everything added,
  untracked files included). It's compared with history but isn't part of it.
- `committed`: a commit was made whose tree is exactly the run's tree, so the run is that commit's record. Changing
  anything before committing, or committing only some of the files, leaves the run wip: it no longer describes what
  was committed. The commit may still exist only locally, an amend or rebase can orphan it.
- `published`: the commit is on a remote branch. Only published runs are history for good.

`sync` moves runs along and uploads them, `test/smoke.sh` and the `post-commit` hook run it. Install the hook once per
clone: `cp hooks/post-commit .git/hooks/ && chmod +x .git/hooks/post-commit`.

**Branches.** A run records its branch and its ancestry, and runs are compared along the commit graph: a run is
compared with the newest committed or published run on its nearest ancestor commit. Branches waiting on SPRT each
compare with the commit they branched from, never with each other. `compare --previous` diffs two runs on the same
branch, the way to see what the last edit changed.

**nyayo.** `git config nyayo.url http://<host>:8001` and `git config nyayo.token <token>` (or `NYAYO_URL` &
`NYAYO_TOKEN`). `sync` uploads every run nyayo doesn't have in its current form, `PUT /api/runs/{id}` with the whole
run, gzipped: a state change is the same call again. Unreachable is fine, sync retries next time. Without a url runs
stay local. CI records no runs, it only runs the unit tests & perft.

## Comparable runs

A position's result is a fact about the commit when the suite is run with one thread at a fixed node count: every
position is searched on its own (new game, cleared TT), so neither the order nor the other positions in the run
matter. Two checks are compared position by position when their id, version, limit, threads, hash and build
(compiler, config, assertions, exceptions, bits) are equal; the same commit built by another compiler can search
differently. A run can hold two checks with the same id and different params, WAC with 1 and with 4 threads. A
position is compared when its id and its EPD file's content hash match. `per_file` only picks the positions, the
smoke tier's 25 per STS theme compare with the same positions in a full run at equal settings.

Multi threaded suites aren't deterministic, `deterministic: false`, they compare totals only. A run of the same tree
and build that differs from an earlier one is reported as not deterministic. Settings from the `EG_EPD_*` environment
mark a check `experiment: true`, it only compares with experiments.

## Format

`test/epd_runner.py --out <file>` writes checks, one JSON line per suite; `test/results.py record` adds the run.
Schema version 1:

```json
{
  "schema_version": 1,
  "run": {
    "id": "fc505f6c-dde8-455b-bc05-a50813801536",   // uuid made when recorded, the upload key
    "kind": "smoke",                       // smoke | extended
    "source": "dev",                       // dev | ci
    "trigger": "local",                    // local | push | schedule | request
    "request_id": null,
    "state": "committed",                  // wip | committed | published
    "commit": "d695c0979c...",             // null while wip
    "base_commit": "148709c3c9...",        // HEAD when the run was made
    "tree": "3477723319c8...",             // the working copy's tree
    "branch": "herd-smoke-report",
    "ancestors": ["d695c0979c...", "..."], // git rev-list --topo-order -n 300, the commit (or base) first
    "version": "1.0.0-herd.10",
    "started_at": "2026-10-14T01:57:01Z", "finished_at": "...", "recorded_at": "...",
    "committed_at": "...", "published_at": "...",
    "host": { "name": "BABYLON", "os": "Windows 11", "cpu": "...", "logical_cores": 24 },
    "build": { "...": "as the checks' build" },
    "ci": { "workflow_run_id": 123, "url": "..." }   // CI only
  },
  "checks": [
    {
      "id": "epd.wac_new", "version": 1, "name": "Smoke_WinAtChess",
      "status": "pass",                    // the harness: pass ran to completion | error | skipped
      "deterministic": true,
      "params": { "limit": { "type": "nodes", "value": 150000 }, "threads": 1, "hash_mb": 64, "per_file": 0,
                  "files": ["wac_new.epd"], "file_sha256": { "wac_new.epd": "..." }, "experiment": false,
                  "workers": 24 },                                     // engine processes, not part of the key
      "metrics": { "solved": { "value": 260 }, "total": { "value": 300 },
                   "points": { "value": 1707 }, "max_points": { "value": 3250 },   // scored suites (STS)
                   "duration": { "value": 14.2, "unit": "s" } },
      "started_at": "2026-10-14T01:57:08+00:00", "finished_at": "2026-10-14T01:57:22+00:00",
      "engine_verdict": { "baseline": 260, "minimum": 252, "repeatable": true, "result": "pass" },
      "build": { "engine": "Elephant Gambit 1.0.0", "version": "1.0.0", "commit": "...", "dirty": false,
                 "branch": "main", "compiler": "msvc 195136260", "config": "release", "assertions": false,
                 "exceptions": false, "bits": 64, "binary_sha256": "..." },   // buildinfo, see above
      "cases": [
        { "id": "WAC.001", "file": "wac_new.epd", "fen": "...", "status": "pass",   // pass | fail | error
          "expected": ["Qg6"], "avoid": [], "got": "g3g6",
          "points": 10, "max_points": 10,                                             // scored suites
          "score": 2, "mate": true, "nodes": 141388, "time_ms": 54, "pv": "g3g6 f7g6 e5g6" }
      ]
    }
  ]
}
```

A case passes when the move played is one of `expected` (bm, SAN) and none of `avoid` (am); `got` is the move played
in coordinate notation. For STS it earns the points of its c0 list, `status` still says whether it was the best
move. `error` is a case the runner couldn't judge: a bm/am it can't read, or an engine that crashed or hung, said in
the case's `error` field. `score`, `mate`, `nodes` & `pv` are the engine's last `info` line, `score` is centipawns or,
with `mate`, moves to mate. `engine_verdict` is the check's baseline & minimum, for information: whether a change is a
regression is worked out from history.

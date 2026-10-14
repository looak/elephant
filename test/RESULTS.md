# EPD results records

Every position the EPD suites search is recorded, so a change can be judged position by position against history:
which positions a branch newly fails or solves, and since which commit a position has been failing. Elo and speed
stay with OpenBench, these records are about behaviour.

## Workflow

```sh
test/smoke.sh                                  # records the smoke run and compares it with its ancestor's
python test/results.py compare                 # the newest run against the nearest ancestor commit's run
python test/results.py compare --against <run> # against a chosen run: file name, commit or tree prefix
python test/results.py case WAC.297            # one position through the history of HEAD
python test/results.py list                    # every recorded run
```

Runs are kept in `logs/results/` (per machine, ignored by git): `history/` for runs that belong to a commit,
`inflight/` for runs on uncommitted changes.

**In flight.** A run on a dirty working tree records the tree its files would commit as (everything added,
untracked files included). It can be compared with history but isn't part of it. When a commit is made whose tree is
exactly that one, the `post-commit` hook promotes the run to that commit's record. Changing anything before
committing, or committing only some of the files, leaves it in flight: it no longer describes what was committed.
Install the hook once per clone: `cp hooks/post-commit .git/hooks/ && chmod +x .git/hooks/post-commit`.

**Branches.** A run records its branch, and runs are compared along the commit graph: a run is compared with the
newest committed run on its nearest ancestor commit. Branches waiting on SPRT each compare with the commit they
branched from, never with each other.

## Comparable runs

A position's result is a fact about the commit when the suite is run with one thread at a fixed node count: every
position is searched on its own (new game, cleared TT), so neither the order nor the other positions in the run
matter. Two checks are compared position by position when their id, version, limit, threads, hash and build
(compiler, config, assertions, exceptions, bits) are equal; the same commit built by another compiler can search
differently. A position is compared when its id and its EPD file's content hash match. `per_file` only picks the
positions, the smoke tier's 25 per STS theme compare with the same positions in a full run at equal settings.

Multi threaded suites aren't deterministic, `deterministic: false`, they compare totals only. A run of the same tree
and build that differs from an earlier one is reported as not deterministic. Settings from the `EG_EPD_*` environment
mark a check `experiment: true`, it only compares with experiments.

## Format

ElephantSuite writes checks with `EG_RESULTS_CHECKS=<file>`, one JSON line per suite; `test/results.py record` adds
the run and the suite files' hashes. The format follows the nyayo results API, schema version 1.

```json
{
  "schema_version": 1,
  "run": {
    "kind": "smoke",                       // smoke | extended
    "trigger": "local",                    // local | push | schedule | request
    "request_id": null,
    "state": "committed",                  // committed | in_flight
    "commit": "148709c3c9...",             // null while in flight
    "base_commit": "148709c3c9...",        // HEAD when the run was made
    "tree": "d23166a6c96d...",             // the working copy's tree
    "branch": "herd-smoke-report",
    "version": "1.0.0-herd.10",
    "started_at": "2026-10-14T01:54:41Z", "finished_at": "...", "recorded_at": "...", "promoted_at": "...",
    "host": { "name": "BABYLON", "os": "Windows 11", "cpu": "...", "logical_cores": 24 },
    "build": { "compiler": "msvc 195136260", "config": "release", "assertions": true, "exceptions": true, "bits": 64 },
    "ci": { "workflow_run_id": 123, "url": "..." }   // CI only
  },
  "checks": [
    {
      "id": "epd.wac_new", "version": 1, "name": "Smoke_WinAtChess",
      "status": "pass",                    // the harness: pass ran to completion | error | skipped
      "deterministic": true,
      "params": { "limit": { "type": "nodes", "value": 150000 }, "threads": 1, "hash_mb": 64, "per_file": 0,
                  "files": ["wac_new.epd"], "file_sha256": { "wac_new.epd": "..." }, "experiment": false },
      "metrics": { "solved": { "value": 260 }, "total": { "value": 300 },
                   "points": { "value": 1707 }, "max_points": { "value": 3250 },   // scored suites (STS)
                   "duration": { "value": 14.2, "unit": "s" } },
      "engine_verdict": { "baseline": 260, "minimum": 252, "repeatable": true, "result": "pass" },
      "build": { "...": "as run.build" },
      "cases": [
        { "id": "WAC.001", "file": "wac_new.epd", "fen": "...", "status": "pass",   // pass | fail | error
          "expected": ["Qg6"], "avoid": [], "got": "g3g6",
          "points": 10, "max_points": 10,                                             // scored suites
          "score": 23996, "mate": true, "nodes": 141388, "time_ms": 54, "pv": "g3g6 f7g6 e5g6" }
      ]
    }
  ]
}
```

A case passes when the move played is one of `expected` (bm) and none of `avoid` (am). For STS it earns the points
of its c0 list, `status` still says whether it was the best move. `error` is a case the harness couldn't run as
written, a FEN or a bm/am the parsers rejected. `engine_verdict` is the suite's own baseline & minimum, for
information: whether a change is a regression is worked out from history.

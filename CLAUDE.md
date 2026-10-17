# Elephant

A UCI chess engine (Elephant Gambit), C++20. Sibling checkouts: `../nyayo` (results history), `../bwawa` (live match
viewer), `../ramani` (specs).

## Specs

The specs for Elephant and the services around it live in ramani (github.com/looak/ramani, private, sibling checkout
at `../ramani`, linked into this folder as `ramani/` on the owner's machine, not tracked). For this repo:
`elephant-engine.md` (UCI, `bench`, `buildinfo`, builds & versioning), `elephant-tests.md` (`ElephantTest`,
`ElephantSuite`) and `test-pipeline.md` (`smoke.sh`, `epd_runner.py`, `results.py`, hooks); `openbench.md` for the
OpenBench side. The README there lists which repo owns which contract.

Each spec has a Current section (as built, the code wins when they disagree) and a Direction section (planned work,
open decisions). When a change here alters a contract (a UCI command or option, the bench, `buildinfo` keys, EPD
tiers, the results records) or settles an open decision, update the matching spec in ramani in the same piece of work.

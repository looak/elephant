# Elephant

A UCI chess engine (Elephant Gambit), C++20. Sibling checkouts: `../nyayo` (results history), `../bwawa` (live match
viewer), `../ramani` (specs).

## Specs

What this repo does is specified here, in `docs/spec/engine.md`, `docs/spec/tests.md` and `docs/spec/test-pipeline.md`:
Purpose, Scope, Current and Interfaces. **Keep it true in the same pull request as the code**: a change that alters a
command, an endpoint, a schema, a file format or anything the spec describes updates the spec in that pull request.
When the code and the spec disagree, the code wins and the spec is fixed.

Where this repo is going lives in ramani (github.com/looak/ramani, private, sibling checkout at `../ramani`): the
README for the system and the contracts table, `elephant-engine.md`, `elephant-tests.md` and `test-pipeline.md` for this
repo's Direction (planned work, open decisions), and `plans/` for work spanning repos. **Read ramani, never edit it**,
nor any other sibling checkout. A pull request that does a plan's step says `Closes looak/<repo>#<sub-issue>`. When it
settles one of ramani's open decisions, say so in the final report. Contracts this repo owns (UCI with `bench` and
`buildinfo`, the bench signature, `test/epd_tiers.json`, phant-indexer's record format in `indexer/README.md`) are
changed only here, by agreement with the owner.

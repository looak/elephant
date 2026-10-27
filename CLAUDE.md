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

## Code conventions

New and changed code follows these; `.clang-tidy` (`readability-identifier-naming`) and `.clang-format` encode them.

- **Naming**: types CamelCase; functions, methods, variables, parameters and constants camelBack (`readPly`,
  `minDepth`, `c_` on a constant is allowed); enum values UPPER_CASE (`Set::WHITE`); namespaces lower_case; macros
  UPPER_CASE; `u8`..`u64`, `i8`..`i64` are the integer aliases. Older code still has PascalCase methods (`UCI`,
  `GameContext`) and a few snake_case functions: don't copy them, and don't rename them inside an unrelated change.
- **Members**: `m_` is a choice, not a rule. A class that keeps its state behind methods and invariants uses `m_` for
  that private state (`GameContext::m_history`). A struct that is its data, read and written directly, has plain
  camelBack public members and no prefix (`ThreadSearchContext`, `PawnInfo`). Within one type, one or the other.
- **Formatting**: the tree isn't formatted by clang-format yet and mixes styles. Format only the lines a change
  touches (`git clang-format origin/main`), never reformat a whole file in passing. 120 columns.
- **Comments** say why, in prose, wrapped by hand at 120: what the code can't say (why a pruning is safe here, what a
  number was fitted on), not what the next line does.
- **clang-tidy** warns only until the naming cleanup is done: run it over the files a change touches
  (`clang-tidy -p build <files>`) and fix what's in your own lines.
- **Renames and cleanups** are their own commits, with the bench line unchanged, never mixed into a search or
  evaluation change waiting on SPRT.

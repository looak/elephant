# A stronger hand-crafted evaluation

Status: plan · 2026-10-15

## Goal

The next piece of work after 1.0.0: a stronger evaluation function. Today's evaluation is PeSTO, material plus
piece-square tables tapered between midgame and endgame. A piece-square table scores a piece on its own square, so
it can't see how pieces and pawns relate to each other. The terms below are the ones it can't express.

## Terms

| Term | Why PeSTO doesn't have it |
|:--|:--|
| King safety | the king table only says where to stand; nothing about the pawn shield, open files near the king or attackers |
| Passed pawns | the pawn table rewards advancement but can't tell a passer from a blocked pawn |
| Doubled, isolated and backward pawns | they depend on the neighbouring pawns |
| Pawn structure in general | chains, phalanxes, connected passers |
| Others, as they come | mobility, bishop pair, rooks on open and semi-open files |

Passed pawns, king safety and mobility are the usual first gains.

## Approach

- **One term per SPRT.** Bundled tests hide each term's value (the three prunings of October were measured as one).
  Bounds `[0, 5]`, as for any gain.
- **Fit the weights, don't hand-set them.** Texel-style fitting on the engine's own game data, offline. Hundreds of
  values are the case where hand-set numbers go wrong.
- **Weights live in a weight store, not in code**, so a new weight list is a test with the same binary and no
  rebuild. Whether the store loads from a file at runtime or is baked in at compile time needs checking first;
  if it's baked in, loading from a file is the small change that makes this and the NNUE work cheap.
- **Pawn structure is cacheable.** A pawn hash table keeps its cost out of the per-node path.

## Related

- Search constants (futility, RFP, LMP, NMP, LMR margins) as UCI options, for SPSA through OpenBench. Floated
  alongside this plan, separate from it: small, around 10 to 15 values, defaults compiled in.
- NNUE trained on the engine's own games is the longer-term evaluation goal in the README. The weight loading
  here is the same plumbing it needs.

## Open questions

- Which term first.
- Whether the weight store loads at runtime today.
- Where the tuning data comes from: self-play at what time control, and how much.

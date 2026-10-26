#include "draw.hpp"

#include <algorithm>
#include <cstdlib>
#include <unordered_set>

#include "polyglot.hpp"

#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>
#include <move/move_executor.hpp>
#include <search/search.hpp>
#include <search/search_constants.hpp>
#include <search/search_threadcontext.hpp>
#include <system/time_manager.hpp>

namespace indexer {
namespace {
    // splitmix64, the same sequence on every platform; std::shuffle's isn't.
    struct Random {
        u64 state;
        u64 next()
        {
            u64 z = (state += 0x9e3779b97f4a7c15ull);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
            return z ^ (z >> 31);
        }
    };

    // a mixed seed for each game, so the games draw independently and a game's sample doesn't depend on the others'.
    u64 gameSeed(u64 seed, u64 game)
    {
        Random mix{ seed ^ (game * 0xd6e8feb86659fd93ull) };
        return mix.next();
    }
} // namespace

struct Drawer::Impl {
    explicit Impl(DrawOptions o)
        : options(o)
        , clock(SearchParameters{}, Set::WHITE)
        , context(Position{}, true, clock)
    {
        context.stopEnabled = false;  // quiescence runs to its own end
    }

    // Not in check, and the engine's own quiescence search finds nothing it would rather play than stand pat: the
    // position is its own evaluation. The search is a test, its line is thrown away.
    bool naturallyQuiet(const Chessboard& board)
    {
        context.position = board.readPosition().copy();
        context.accumulator = PestoAccumulator::computeFromScratch(context.position.read().material());
        context.gameState.whiteToMove = board.readToPlay() == Set::WHITE;

        PVLine line;
        const i16 score = context.gameState.whiteToMove ? Search::quiescenceLine<Set::WHITE>(context, line)
                                                         : Search::quiescenceLine<Set::BLACK>(context, line);
        return line.length == 0 && std::abs(score) < c_checkmateMinScore;
    }

    DrawOptions options;
    TimeManager clock;
    ThreadSearchContext context;
    std::unordered_set<u64> seen;   // the Polyglot hashes written so far
    u64 gamesSeen = 0;
};

Drawer::Drawer(DrawOptions options) : m_impl(std::make_unique<Impl>(options)) {}
Drawer::~Drawer() = default;

std::string resultLabel(const std::string& result)
{
    if (result == "1-0")
        return "[1.0]";
    if (result == "0-1")
        return "[0.0]";
    if (result == "1/2-1/2")
        return "[0.5]";
    return "";
}

void Drawer::draw(const GameIndex& entry, std::string& out)
{
    Impl& impl = *m_impl;
    const u64 game = impl.gamesSeen++;
    ++m_stats.games;

    const std::string label = resultLabel(entry.result);
    // a game that didn't read has no positions to trust; one with a null move was never played over the board.
    if (!entry.ok || label.empty() || entry.played.size() != entry.plies || entry.positions.size() != entry.plies + 1)
        return;
    if (std::any_of(entry.played.begin(), entry.played.end(), [](PackedMove move) { return move == PackedMove::NullMove(); }))
        return;

    Chessboard board;
    try {
        if (!io::fen_parser::deserialize(entry.startFen, board))
            return;
    }
    catch (const std::exception&) {
        return;
    }

    ++m_stats.drawn;
    struct Candidate {
        u32 ply;
        std::string fen;
    };
    std::vector<Candidate> candidates;

    // the positions after ply 0, 1, ... in turn, the ones from minPly on that are worth a look.
    MoveUndoUnit undo;
    u32 candidateCount = 0;
    for (u32 ply = 0;; ++ply) {
        if (ply >= impl.options.minPly) {
            const PositionFacts facts = positionFacts(board);
            if (!facts.inCheck && facts.canMove) {
                ++candidateCount;
                if (!impl.options.quiet || impl.naturallyQuiet(board)) {
                    std::string fen;
                    io::fen_parser::serialize(board, fen);
                    candidates.push_back({ ply, std::move(fen) });
                }
            }
        }
        if (ply == entry.plies)
            break;
        MoveExecutor executor(board.editPosition());
        GameState& state = board.editState();
        executor.makeMove<true>(entry.played[ply], undo, state.plyCount);
        state.whiteToMove = !state.whiteToMove;
        state.moveCount += static_cast<u16>(state.whiteToMove);
    }

    m_stats.candidates += candidateCount;
    m_stats.quiet += candidates.size();
    m_stats.quietPerGame.push_back(static_cast<u32>(candidates.size()));

    // a seeded shuffle, then the first that no game has given yet.
    Random random{ gameSeed(impl.options.seed, game) };
    for (size_t i = candidates.size(); i > 1; --i)
        std::swap(candidates[i - 1], candidates[random.next() % i]);

    u32 written = 0;
    for (const Candidate& candidate : candidates) {
        if (written == impl.options.perGame)
            break;
        if (!impl.seen.insert(entry.positions[candidate.ply]).second) {
            ++m_stats.duplicates;
            continue;
        }
        out += candidate.fen;
        out += ' ';
        out += label;
        out += '\n';
        ++written;
    }
    m_stats.written += written;
}

} // namespace indexer

#pragma once
#include <system/platform.hpp>

#include <string>
#include <vector>

namespace ElephantTest {

/**
 * @brief Perft positions with their node count at every depth, shared by the unit tests & ElephantSuite. Each runs
 * a position at the deepest depth within its node budget:
 *     ElephantTest   PerftShallow                     up to c_perftShallowBudget nodes, milliseconds
 *     ElephantSuite  PerftFixture.ReferencePositions  up to c_perftReferenceBudget nodes, seconds
 *     ElephantSuite  PerftFixture.DeepPositions       the deepest listed depth, past the reference budget, a minute
 *
 * The six standard positions are from https://www.chessprogramming.org/Perft_Results, every depth published. The
 * others list their published count as the deepest depth, the shallower ones are the engine's own, written down once
 * the deep counts matched.    */
struct PerftPosition {
    std::string name;
    std::string fen;
    std::vector<u64> nodes; // nodes[depth - 1]

    int maxDepth() const { return static_cast<int>(nodes.size()); }
    u64 nodesAt(int depth) const { return nodes[static_cast<size_t>(depth - 1)]; }

    // the deepest depth with no more than budget nodes, 0 when even depth 1 is over it.
    int deepestWithin(u64 budget) const {
        int depth = 0;
        while (depth < maxDepth() && nodesAt(depth + 1) <= budget)
            depth++;
        return depth;
    }
};

inline constexpr u64 c_perftShallowBudget = 1'000'000;
inline constexpr u64 c_perftReferenceBudget = 250'000'000;

inline const std::vector<PerftPosition>& perftPositions()
{
    static const std::vector<PerftPosition> positions = {
        // the standard positions
        { "start position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            { 20, 400, 8902, 197281, 4865609, 119060324, 3195901860 } },
        { "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
            { 48, 2039, 97862, 4085603, 193690690, 8031647685 } },
        { "position 3, rook & pawns", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
            { 14, 191, 2812, 43238, 674624, 11030083, 178633661, 3009794393 } },
        { "position 4, promotions & castling", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
            { 6, 264, 9467, 422333, 15833292, 706045033 } },
        { "position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
            { 44, 1486, 62379, 2103487, 89941194 } },
        { "position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
            { 46, 2079, 89890, 3894594, 164075551, 6923051137 } },

        // special cases
        { "illegal en passant", "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1",
            { 18, 92, 1670, 10138, 185429, 1134888 } },
        { "illegal en passant", "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1",
            { 13, 102, 1266, 10276, 135655, 1015133 } },
        { "en passant capture, checks opponent", "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1",
            { 15, 126, 1928, 13931, 206379, 1440467 } },
        { "short castling", "5k2/8/8/8/8/8/8/4K2R w K - 0 1",
            { 15, 66, 1198, 6399, 120330, 661072 } },
        { "long castling", "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1",
            { 16, 71, 1286, 7418, 141077, 803711 } },
        { "castling rights", "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1",
            { 26, 1141, 27826, 1274206 } },
        { "castling prevented", "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1",
            { 44, 1494, 50509, 1720476 } },
        { "promotion out of check", "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1",
            { 11, 133, 1442, 19174, 266199, 3821001 } },
        { "discovered check", "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1",
            { 29, 165, 5160, 31961, 1004658 } },
        { "promote to give check", "4k3/1P6/8/8/8/8/K7/8 w - - 0 1",
            { 9, 40, 472, 2661, 38983, 217342 } },
        { "under promote to give check", "8/P1k5/K7/8/8/8/8/8 w - - 0 1",
            { 6, 27, 273, 1329, 18135, 92683 } },
        { "self stalemate", "K1k5/8/P7/8/8/8/8/8 w - - 0 1",
            { 2, 6, 13, 63, 382, 2217 } },
        { "stalemate and checkmate", "8/k1P5/8/1K6/8/8/8/8 w - - 0 1",
            { 10, 25, 268, 926, 10857, 43261, 567584 } },
        { "stalemate and checkmate", "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1",
            { 37, 183, 6559, 23527 } },
        { "bishop vs rook endgame", "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1",
            { 13, 284, 3529, 85765, 1063513 } },
    };
    return positions;
}

} // namespace ElephantTest

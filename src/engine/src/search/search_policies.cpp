#include <search/search_policies.hpp>
#include <search/search_constants.hpp>
#include <search/search_heuristic_structures.hpp>
#include <search/transposition_table.hpp>

#include <move/generation/move_ordering_view.hpp>

#include <optional>

namespace search_policies {

// --- Transposition Table Policies ---
void TT::assign(TranspositionTable& tt) {
    m_table = &tt;        
}

std::optional<i16> TT::probe(u64 hash, u16 requiredDepth, i16 alpha, i16 beta, TranspositionFlag& flag, PackedMove& outMove) {        
    i16 score;
    u8 depth;
    if (m_table->probe(hash, outMove, score, depth, flag) == false)
        return std::nullopt;

    if (depth < requiredDepth)
        return std::nullopt;

    // mate score is already adjusted during search, so we don't need to adjust it again here.

    switch (flag) {
        case TTF_CUT_EXACT:
            return score;
        case TTF_CUT_BETA:
            if (score >= beta)
                return score;
            break;
        case TTF_CUT_ALPHA:
            if (score <= alpha)
                return score;
            break;
        default:
            break;
    }

    return std::nullopt;
}

bool TT::probeMove(u64 hash, PackedMove& outMove) {
    i16 dummyScore;
    u8 dummyDepth;
    TranspositionFlag dummyFlag;
    return m_table->probe(hash, outMove, dummyScore, dummyDepth, dummyFlag);
}

void TT::update(u64 hash, const PackedMove& move, i16 score, u8 depth, const TranspositionFlag& flag)
{
    m_table->store(hash, move, score, depth, flag);
}

void TT::printStats() 
{
#ifdef DEBUG_TRANSITION_TABLE
    m_table->print_stats();
#endif
}

// --- Late Move Reduction (LMR) Policies ---
bool LMR::shouldReduce(u32 depth, const PackedMove& move, u16 index, bool isChecked, bool /*isChecking */) {
    return depth > lmr_params::minDepth 
    && (move.isQuiet() || index > lmr_params::reduceAfterIndex)
    && isChecked == false;
    //&& isChecking == false;
    }

u8 LMR::getReduction(u8 depth) {
    u8 reduction = 1;
    if (depth > lmr_params::earlyReductionThreshold) reduction++;
    if (depth == 0) return 0;
    return std::min(reduction, (u8)(depth - 1));
}

// --- Move Ordering Heuristics (Killers/History) Policies ---
void MoveOrdering::push(KillerMoves& killers, PackedMove move, u16 ply) {
        if (move.isQuiet() == false)
            return; // Only store quiet moves as killers
        killers.push(move, ply);
}
void MoveOrdering::prime(const KillerMoves& killers, MoveOrderingView& view, u16 ply) {
        killers.retrieve(ply, view.killers[0], view.killers[1]);
}

u8 QuiescencePolicy::maxDepth = quiescence_params::defaultMaxDepth;


// --- Debug Policies ---

#if defined(DEVELOPMENT_BUILD)
class DebugEnabled {
public:
    static const Clock& pushClock() {
        Clock clock;
        clock.Start();
        m_searchClocks.push(clock);
        return m_searchClocks.top();
    }

    static void popClock() {
        if (!m_searchClocks.empty()) {
            Clock& clock = m_searchClocks.top();
            clock.Stop();            
            m_searchClocks.pop();
        }
    }

    static void reportNps(u64 nodes, u64 qnodes) {
        const Clock& clock = m_searchClocks.top();

        std::cout << " ------------------------------ \n";
        std::cout << " Nodes: " << io::printer::formatReadableNumber(nodes)
                  << " QNodes: " << io::printer::formatReadableNumber(qnodes)
                  << " Total: " << io::printer::formatReadableNumber(nodes + qnodes) << "\n";
        std::cout << " NPS:   " << io::printer::formatReadableNumber(clock.calcNodesPerSecond(nodes))
                  << " QNPS: " << io::printer::formatReadableNumber(clock.calcNodesPerSecond(qnodes)) << "\n";
        std::cout << " Total NPS: " << io::printer::formatReadableNumber(clock.calcNodesPerSecond(nodes + qnodes)) << "\n";
    }

private:
    static inline std::stack<Clock> m_searchClocks;
};
#endif


} // namespace search_policies
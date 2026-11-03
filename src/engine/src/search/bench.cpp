#include <search/bench.hpp>

#include <algorithm>

#include <core/game.hpp>
#include <search/monad.hpp>
#include <search/search_request.hpp>
#include <system/clock.hpp>

namespace bench {

u64 Result::nps() const
{
    return nodes * 1000 / static_cast<u64>(std::max<i64>(elapsedMs, 1));
}

Result run(const std::vector<std::string>& positions, u32 searchDepth, size_t hashMb, SearchListener& listener)
{
    // one set of tables for every position, cleared like a new game. A fresh one per position allocated a new TT each time
    // and the bench time was mostly the OS mapping that memory in, not the search. A cleared TT is all zero with age 0, the
    // same as a new one, the node count doesn't change. Created before the clock.
    Game game;
    Monad monad(hashMb);

    Clock timer;
    timer.Start();
    u64 nodes = 0;
    for (const std::string& fen : positions) {
        if (!game.loadFen(fen))
            continue;
        monad.newGame();

        SearchLimits limits;
        limits.SearchDepth = static_cast<u8>(searchDepth);
        nodes += monad.search(makeSearchRequest(game, limits), listener).count;
    }
    timer.Stop();

    return { nodes, timer.getElapsedTime() };
}

Result runSignature(SearchListener& listener)
{
    // 16 MB rather than more: each position clears the whole table and OpenBench benches every worker thread at once,
    // clearing 64 MB 26 times made the bench measure memory bandwidth. The depth 7 searches don't fill 16 MB, the node
    // count is the same.
    return run(fens, depth, 16, listener);
}

} // namespace bench

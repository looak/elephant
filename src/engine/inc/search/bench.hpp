#pragma once
#include <string>
#include <vector>

#include <search/bench_positions.hpp>
#include <search/search_listener.hpp>
#include <system/platform.hpp>

namespace bench {

struct Result {
    u64 nodes = 0;
    i64 elapsedMs = 0;
    // a bench under a millisecond counts as one.
    [[nodiscard]] u64 nps() const;
};

// Searches positions to searchDepth on one thread, every one as a new game: the table & the thread's tables are zeroed in
// place between them, so a position's nodes don't depend on the others. Only the searching is timed. A FEN that doesn't
// read is skipped. Holds its own tables, whoever calls it keeps theirs.
Result run(const std::vector<std::string>& positions, u32 searchDepth, size_t hashMb, SearchListener& listener);

// The bench signature: bench::fens to bench::depth. Every binary's bench is this one, printed its own way.
Result runSignature(SearchListener& listener);

} // namespace bench

#pragma once
#include <future>
#include <memory>
#include <optional>
#include <vector>

#include <search/search_listener.hpp>
#include <search/search_request.hpp>
#include <search/search_results.hpp>
#include <search/state/transposition_table.hpp>
#include <system/time_manager.hpp>

class Worker;

// Hash's default, in megabytes.
inline constexpr size_t c_defaultHashMb = 8;

// One independent mind, after Leibniz's monads: its own memory, nothing shared with another Monad, no colour of its own.
// Each sees the whole board from what it remembers, so self-play is one game and two of these. It owns what outlives a
// search, the transposition table & the Workers with every thread's tables, and answers one SearchRequest at a time. The
// game isn't here, a request is a complete copy of the question, whoever holds the game can change it while a search
// runs.
//
// Every method is called from one thread, the one reading commands. The search runs on its own, a listener hears about it
// on those threads.
class Monad {
public:
    explicit Monad(size_t hashMb = c_defaultHashMb, u16 threadCount = 1);
    // A search still running is stopped & waited for.
    ~Monad();
    Monad(const Monad&) = delete;
    Monad& operator=(const Monad&) = delete;

    // A search reads these and a table can be freed under it, so each ends a running search first.
    void setThreads(u16 count);
    void setHash(size_t megabytes);
    void setMoveOverhead(u32 milliseconds);

    // Games are independent: zeroes the table & every thread's history tables, keeping the memory. Ends a running search.
    void newGame();

    // Answers request, returning immediately. A search still running is ended first. listener has to outlive the search.
    //
    // listener hears exactly one of onBestMove or onSearchFailed when it's over. An infinite search runs until stopped:
    // one that ends on its own, on a forced mate or at the ply limit, keeps its answer until stop(). UCI asks this of
    // go infinite & go ponder.
    void go(SearchRequest request, SearchListener& listener);

    // Answers request on the calling thread and returns the result, for whoever just wants the answer. A search still
    // running is ended first. listener hears the iterations, not onBestMove, and nothing can stop this search: a limit
    // has to end it, an infinite one runs to the ply limit.
    SearchResult search(const SearchRequest& request, SearchListener& listener = noSearchListener());

    // Asks the running search to end & returns at once, wait() or the listener tell when it has.
    void stop();

    // Blocks until the search is over, its result. An infinite search needs a stop() to get there. Without a search to
    // wait for, an empty result.
    SearchResult wait();

    // for tests, what a search left behind.
    TranspositionTable& editTranspositionTable() { return m_tt; }

private:
    void endSearch();
    void startClock(const SearchRequest& request);
    // one thread per Worker until the main one is done, on the clock startClock made.
    SearchResult runWorkers(const SearchRequest& request, SearchListener& listener);
    [[nodiscard]] u16 threadCount() const { return static_cast<u16>(m_workers.size()); }

    TranspositionTable m_tt;
    std::vector<std::unique_ptr<Worker>> m_workers;
    u32 m_moveOverheadMs = c_defaultMoveOverhead_ms;
    // Made for each go on the thread that reads commands, a stop right after go can't be lost and one from an earlier
    // search can't carry over. Replaced only once the search reading it is over.
    std::optional<TimeManager> m_clock;
    std::future<SearchResult> m_search;
};

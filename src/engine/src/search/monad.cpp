#include <search/monad.hpp>

#include <chrono>
#include <exception>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

#include <search/worker.hpp>
#include <diagnostics/logger.hpp>

Monad::Monad(size_t hashMb, u16 threadCount) : m_tt(hashMb)
{
    setThreads(threadCount);
}

Monad::~Monad()
{
    endSearch();
}

void Monad::setThreads(u16 count)
{
    endSearch();
    if (threadCount() == count)
        return;  // the tables carry over.
    m_workers.clear();
    for (u16 i = 0; i < count; ++i)
        m_workers.push_back(std::make_unique<Worker>());
}

void Monad::setHash(size_t megabytes)
{
    endSearch();
    m_tt.resize(megabytes, threadCount());
}

void Monad::setMoveOverhead(u32 milliseconds)
{
    endSearch();
    m_moveOverheadMs = milliseconds;
}

void Monad::newGame()
{
    endSearch();
    m_tt.clear(threadCount());
    for (const auto& worker : m_workers)
        worker->newGame();
}

void Monad::go(SearchRequest request, SearchListener& listener)
{
    endSearch();
    startClock(request);

    m_search = std::async(std::launch::async, [this, request = std::move(request), &listener]() -> SearchResult {
        try {
            const auto start = std::chrono::steady_clock::now();
            SearchResult result = runWorkers(request, listener);
            // when the search ended, not when it was told it could answer.
            const u64 elapsedMs = static_cast<u64>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());

            if (request.limits.Infinite) {
                const std::stop_token stopped = m_clock->cancelToken();
                while (!stopped.stop_requested())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            listener.onBestMove(result, elapsedMs);
            return result;
        }
        catch (const std::exception& e) {
            listener.onSearchFailed(e);
            return SearchResult{};
        }
    });
}

SearchResult Monad::search(const SearchRequest& request, SearchListener& listener)
{
    endSearch();
    startClock(request);
    return runWorkers(request, listener);
}

void Monad::startClock(const SearchRequest& request)
{
    m_clock.emplace(request.limits, request.toPlay);
    m_clock->setMoveOverhead(m_moveOverheadMs);
}

SearchResult Monad::runWorkers(const SearchRequest& request, SearchListener& listener)
{
    m_clock->begin();
    m_tt.incrementAge();

    SharedSearchState shared;
    const u16 count = threadCount();
    std::vector<std::future<SearchResult>> searches;
    for (u16 threadId = 0; threadId < count; ++threadId) {
        searches.push_back(std::async(std::launch::async, [this, &request, &shared, &listener, count, threadId]() {
            return m_workers[threadId]->run(request, *m_clock, m_tt, shared, listener, threadId, count);
        }));
    }

    // the main thread's result is played. get() blocks until a thread is finished & propagates its exceptions.
    SearchResult finalResult;
    for (size_t threadId = 0; threadId < searches.size(); ++threadId) {
        try {
            SearchResult result = searches[threadId].get();
            if (threadId == 0)
                finalResult = result;
        } catch (const std::exception& e) {
            LOG_ERROR("Search thread exception: {}", e.what());
        }
        // also when the main thread threw, the helpers mustn't keep searching.
        if (threadId == 0)
            shared.stop.store(true, std::memory_order_relaxed);
    }
    finalResult.count = shared.nodes.load(std::memory_order_relaxed);
    return finalResult;
}

void Monad::stop()
{
    if (m_clock)
        m_clock->cancel();
}

SearchResult Monad::wait()
{
    return m_search.valid() ? m_search.get() : SearchResult{};
}

void Monad::endSearch()
{
    stop();
    (void)wait();
}

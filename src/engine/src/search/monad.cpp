#include <search/monad.hpp>

#include <chrono>
#include <exception>
#include <stop_token>
#include <thread>
#include <utility>

#include <search/search.hpp>

Monad::Monad() : m_tt(c_defaultHashMb) {}

Monad::~Monad()
{
    endSearch();
}

void Monad::setThreads(u16 count)
{
    endSearch();
    m_searchContext.initialize(count);
}

void Monad::setHash(size_t megabytes)
{
    endSearch();
    m_tt.resize(megabytes, m_searchContext.threadCount());
}

void Monad::setMoveOverhead(u32 milliseconds)
{
    endSearch();
    m_moveOverheadMs = milliseconds;
}

void Monad::newGame()
{
    endSearch();
    m_tt.clear(m_searchContext.threadCount());
    m_searchContext.reset();
}

void Monad::go(SearchRequest request, SearchListener& listener)
{
    endSearch();
    m_clock.emplace(request.limits, request.toPlay);
    m_clock->setMoveOverhead(m_moveOverheadMs);

    m_search = std::async(std::launch::async, [this, request = std::move(request), &listener]() -> SearchResult {
        try {
            Search search(m_searchContext, m_tt, listener);
            const auto start = std::chrono::steady_clock::now();
            SearchResult result = search.go(request, *m_clock);
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

#pragma once
#include <atomic>

#include <search/search_listener.hpp>

// Writes a search as UCI: its iterations as info lines, its end as bestmove. The uci logger locks per line, so threads
// can share one.
class UciSearchListener final : public SearchListener {
public:
    // reportHelpers: uci debug on, the helper threads also report their iterations as info strings. Otherwise only the
    // main thread is reported.
    explicit UciSearchListener(bool reportHelpers = false) : m_reportHelpers(reportHelpers) {}

    // debug on|off can arrive during a search.
    void setReportHelpers(bool reportHelpers) { m_reportHelpers = reportHelpers; }

    void onIteration(const IterationInfo& info) override;
    void onBestMove(const SearchResult& result, u64 elapsedMs) override;
    // The GUI waits for a bestmove whatever happened: info string error, then bestmove 0000.
    void onSearchFailed(const std::exception& e) override;

private:
    std::atomic<bool> m_reportHelpers;
};

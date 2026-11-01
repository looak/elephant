#pragma once
#include <search/search_listener.hpp>

// Writes a search's iterations as UCI info lines. The uci logger locks per line, so threads can share one.
class UciSearchListener final : public SearchListener {
public:
    // reportHelpers: uci debug on, the helper threads also report their iterations as info strings. Otherwise only the
    // main thread is reported.
    explicit UciSearchListener(bool reportHelpers = false) : m_reportHelpers(reportHelpers) {}

    void onIteration(const IterationInfo& info) override;

private:
    bool m_reportHelpers;
};

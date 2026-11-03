#pragma once
#include <search/search_results.hpp>
#include <system/platform.hpp>

// A completed iteration of one search thread. pv is only valid during the call.
struct IterationInfo {
    u16 threadId = 0;
    u32 depth = 0;
    i32 score = 0;
    // the main thread reports the nodes of all threads, so nodes / time is the real nps. A helper reports its own.
    u64 nodes = 0;
    u64 timeMs = 0;
    // only worked out for the main thread, 0 from the helpers.
    u32 hashfull = 0;
    const PVLine& pv;
};

// How search tells the outside what it's doing, it never prints. Called on the search threads, every thread of a search
// calls the same listener, so an implementation has to be safe to call concurrently.
class SearchListener {
public:
    virtual ~SearchListener() = default;
    virtual void onIteration(const IterationInfo& info) = 0;
};

// For searches nobody is watching, the default.
class NullSearchListener final : public SearchListener {
public:
    void onIteration(const IterationInfo&) override {}
};

// Holds no state, so one instance can serve every search.
inline SearchListener& noSearchListener() {
    static NullSearchListener listener;
    return listener;
}

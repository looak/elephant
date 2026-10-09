#pragma once

#include <chrono>
#include <stop_token>

#include <material/chess_piece_defines.hpp>

struct SearchParameters;

using timepoint_t = std::chrono::high_resolution_clock::time_point;
using ms_t = std::chrono::milliseconds;
using chess_time_t = std::chrono::high_resolution_clock;

// time (ms) kept in reserve on every move for latency between engine and GUI.
inline constexpr u32 c_defaultMoveOverhead_ms = 10;
inline constexpr u32 c_maxMoveOverhead_ms = 5000;

// moves the clock is planned to last when the GUI doesn't send movestogo.
inline constexpr u32 c_defaultMovesToGo = 20;
// the soft limit is this share (percent) of the target time, the iteration started just before it runs past it.
inline constexpr u32 c_softLimit_pct = 60;
// the hard limit is this many times the target time.
inline constexpr u32 c_hardLimitFactor = 3;
// share (percent) of the clock, after overhead, a single move may ever use.
inline constexpr u32 c_maxClockUsage_pct = 75;

class TimeManager {
public:
    /**
     * @brief Constructs the TimeManager with the rules for this search.
     * @param params The SearchParameters (movetime, wtime, binc, etc.) from UCI.
     * @param perspective The color we are playing (Set::WHITE or Set::BLACK).   */
    TimeManager(const SearchParameters& params, Set perspective);


    /**
     * @brief Applies the time settings from the SearchParameters.
     * @param params The SearchParameters (movetime, wtime, binc, etc.) from UCI.
    * @param perspective The color we are playing (Set::WHITE or Set::BLACK).   */
    void applyTimeSettings(const SearchParameters& params, Set perspective);

    /**
     * @brief Sets the time reserved on every move for communication latency, persists across searches.
     * @param overhead_ms Milliseconds subtracted from the clock before allocating search time.   */
    void setMoveOverhead(u32 overhead_ms);

    /**
     * @brief Checks if another iteration of iterative deepening should be started.
     * @return true while within the soft limit, false otherwise.     */
    bool continueIterativeDeepening() const;

        /**
     * @brief Call this *right before* starting the search to set timers.     */
    void begin();

    /**
     * @brief Gets the current time in milliseconds since begin.
     * @return Current time in milliseconds.     */
    u64 now() const;

    /**
     * @brief Time after which no new iteration is started, valid after begin().
     * @return Soft limit in milliseconds since begin.     */
    u64 softLimit() const;

    /**
     * @brief Time at which the search is aborted, valid after begin().
     * @return Hard limit in milliseconds since begin.     */
    u64 hardLimit() const;

    /**
     * @brief The main function for search threads to call periodically.
     * @return true if the search should stop, false otherwise.    */
    bool shouldStop() const;

        /**
     * @brief Signals all search threads to stop (e.g., from a UCI "stop" command).     */
    void cancel(); 

    /**
     * @brief Gets the stop token associated with this TimeManager.
     * @return The std::stop_token used to signal search threads to stop.     */
    std::stop_token cancelToken() const;
    /**
     * @brief Resets the TimeManager state for a new search.
     * This clears any stop requests and prepares the manager for reuse.     */
    void reset();

private:
    // --- Configuration (set at creation) ---
    u32 m_timeLeft_ms;
    u32 m_increment_ms;
    u32 m_moveTime_ms;
    u32 m_movesToGo;
    u32 m_moveOverhead_ms = c_defaultMoveOverhead_ms;
    u64 m_softLimit_ms = 0;
    u64 m_hardLimit_ms = 0;

    // --- State ---
    timepoint_t m_startTime;
    timepoint_t m_endTime;
    std::stop_source m_stopSource;
    bool m_isTimeManaged;

    /**
     * @brief This is the "brain" of the time manager, calculates the soft & hard limits of the search.  */
    void calculateLimits();

};
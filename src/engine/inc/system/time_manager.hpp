#pragma once

#include <chrono>
#include <stop_token>

#include <material/chess_piece_defines.hpp>

struct SearchParameters;

using timepoint_t = std::chrono::high_resolution_clock::time_point;
using ms_t = std::chrono::milliseconds;
using chess_time_t = std::chrono::high_resolution_clock;

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
     * @brief Checks if another iteration of iterative deepening should be started.
     * @param last_iteration_time_ms The time (in ms) that the *previous* iteration took.
     * @return true if the engine should search deeper, false otherwise.     */
    bool continueIterativeDeepening(u64 lastIterationTimeSpan) const;

        /**
     * @brief Call this *right before* starting the search to set timers.     */
    void begin();

    /**
     * @brief Gets the current time in milliseconds since begin.
     * @return Current time in milliseconds.     */
    u64 now() const;

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

    // --- State ---
    timepoint_t m_startTime;
    timepoint_t m_endTime;
    std::stop_source m_stopSource;
    bool m_isTimeManaged;

    /**
     * @brief This is the "brain" of the time manager, calculating the ideal time slice.
     * @return The optimal time (in ms) to allocate for the current search.     */
    u64 calculateSearchTime();

};
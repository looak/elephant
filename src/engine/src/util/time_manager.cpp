#include <util/time_manager.hpp>
#include <search/search.hpp>

TimeManager::TimeManager(const SearchParameters& params, Set perspective)
    : m_perspective(perspective)
    , m_stopRequested(false)
    , m_isTimeManaged(true)
{
    applyTimeSettings(params, perspective);
}

void TimeManager::applyTimeSettings(const SearchParameters& params, Set perspective) {
        // Set up the time parameters for our side
    if (perspective == Set::WHITE) {
        m_timeLeft_ms = params.WhiteTimelimit;
        m_increment_ms = params.WhiteTimeIncrement;
    } else {
        m_timeLeft_ms = params.BlackTimelimit;
        m_increment_ms = params.BlackTimeIncrement;
    }
    
    m_moveTime_ms = params.MoveTime;
    m_movesToGo = params.MovesToGo;

    // Check for non-time-managed modes (infinite, depth-only, etc.)
    if (params.Infinite || params.SearchDepth == 0) {
        m_isTimeManaged = false;
    } 
    else if (params.SearchDepth > 0 && params.SearchDepth < c_maxSearchDepth) {
        // m_isTimeManaged = false; // Depth-limited search
    }
    else if (params.MoveTime > 0) {
        m_isTimeManaged = true; // Special case: fixed movetime
    }
    else if (m_timeLeft_ms == 0) {
        m_isTimeManaged = false; // No wtime/btime, must be infinite
    }
}

    u64 TimeManager::calculateSearchTime() {
    if (!m_isTimeManaged)
        return std::numeric_limits<u64>::max(); // effectively infinite time

    u64 allocatedTime = 0;

    if (m_moveTime_ms > 0) {
        allocatedTime = static_cast<u64>(m_moveTime_ms);
    } else {
        // Basic heuristic: allocate a fraction of remaining time
        u64 baseTime = m_timeLeft_ms / 24; // e.g., 1/24th of remaining time

        // Consider moves to go if specified
        if (m_movesToGo > 0) {
            baseTime = m_timeLeft_ms / m_movesToGo;
        }

         // Add increment - partial if available
        baseTime += m_increment_ms * 0.75f;
        allocatedTime = baseTime;
    }

    // Safety margin to avoid time forfeits
    allocatedTime = static_cast<u64>(allocatedTime * 0.95f);
    return allocatedTime;
}

bool TimeManager::continueIterativeDeepening(u64 lastIterationTimeSpan) const {
    if (m_isTimeManaged == false) {
        // If not time-managed (e.g., depth-limited), always allow.
        // The iterative deepener loop will handle the depth limit.
        return true; 
    }

    // A common, safe heuristic: if the *next* iteration is
    // predicted to take more than 50% of the *total remaining time*, stop.
    // A simple prediction is that the next depth will take ~4-6x longer.
    // We'll use a factor of 4 for safety.    
    i64 predictedTime = lastIterationTimeSpan * 4;

    timepoint_t now = chess_time_t::now();
    auto timeSpent = std::chrono::duration_cast<ms_t>(now - m_startTime).count();
    auto allocatedTime = std::chrono::duration_cast<ms_t>(m_endTime - m_startTime).count();
    const float margin = 0.95f;
    i64 timeRemaining = (allocatedTime - timeSpent) * margin;

    // If we predict the next iteration will use up more than our remaining time and a margin, stop.
    return predictedTime < timeRemaining;
}

void TimeManager::begin() {
    m_startTime = chess_time_t::now();
    m_stopRequested.store(false, std::memory_order_relaxed);

    if (m_isTimeManaged == false) {
        // For infinite, depth, or nodes search, set stop time to "never"
        m_endTime = timepoint_t::max();
    } 
    else {
        // We have time controls, calculate the stop time.
        i64 allocation_ms = calculateSearchTime();
        m_endTime = m_startTime + ms_t(allocation_ms);
    }
}

u64 TimeManager::now() const {
    timepoint_t currentTime = chess_time_t::now();
    auto elapsed = std::chrono::duration_cast<ms_t>(currentTime - m_startTime).count();
    return static_cast<u64>(elapsed);
}

bool TimeManager::shouldStop() const {
    // Check for external "stop" command (e.g., from UCI)
    if (m_stopRequested.load(std::memory_order_relaxed)) {
        return true;
    }

    // For non-timed searches, we never stop based on time.
    if (m_isTimeManaged == false) {
        return false;
    }

    // This is the main time check.
    // The search loop itself is responsible for deciding *how often* to call this.
    return chess_time_t::now() > m_endTime;
}

void TimeManager::cancel() {
    m_stopRequested.store(true, std::memory_order_release);
}
#include <util/time_manager.hpp>
#include <search/search.hpp>

TimeManager::TimeManager(const SearchParameters& params, Set perspective)
    : m_isTimeManaged(true)
{
    applyTimeSettings(params, perspective);
}

void TimeManager::applyTimeSettings(const SearchParameters& params, Set perspective) {
    m_isTimeManaged = false;

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

    if (params.MoveTime > 0 || m_timeLeft_ms > 0) {
        m_isTimeManaged = true;
    }

    if (params.Infinite) {
        m_isTimeManaged = false;
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
        baseTime += (m_increment_ms * 75) / 100; // use 75% of increment
        allocatedTime = baseTime;
    }

    // Safety margin to avoid time forfeits
    u32 margin = 98; // 98% of calculated time    
    allocatedTime = (allocatedTime * margin) / 100;
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
    u64 predictedTime = lastIterationTimeSpan * 4;

    timepoint_t now = chess_time_t::now();
    i64 timeSpent = std::chrono::duration_cast<ms_t>(now - m_startTime).count();
    i64 allocatedTime = std::chrono::duration_cast<ms_t>(m_endTime - m_startTime).count();

    if (timeSpent >= allocatedTime)
        return false; // already out of time

    const u32 margin = 95; // 95% margin to be safe
    u64 timeRemaining = (u64)((allocatedTime - timeSpent) * margin) / 100;

    // If we predict the next iteration will use up more than our remaining time and a margin, stop.
    return predictedTime < timeRemaining;
}

void TimeManager::begin() {
    m_startTime = chess_time_t::now();    

    if (m_isTimeManaged == false) {
        // For infinite, depth, or nodes search, set stop time to "never"
        m_endTime = timepoint_t::max();
    } 
    else {
        // We have time controls, calculate the stop time.
        u64 allocation_ms = calculateSearchTime();
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
    if (m_stopSource.stop_requested()) {
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
    m_stopSource.request_stop();
}

std::stop_token TimeManager::cancelToken() const {
    return m_stopSource.get_token();
}

void TimeManager::reset() {
    // Clear any previous stop requests
    this->cancel();
    m_stopSource = std::stop_source();
}
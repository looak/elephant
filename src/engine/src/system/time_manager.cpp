#include <system/time_manager.hpp>
#include <search/search.hpp>

#include <algorithm>
#include <limits>

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

void TimeManager::calculateLimits() {
    // Never plan beyond what is actually on the clock, the increment is only credited after we move
    // and the GUI's view of our clock includes the latency between us.
    auto minusOverhead = [this](u64 time) -> u64 {
        return time > m_moveOverhead_ms ? time - m_moveOverhead_ms : 1;
    };

    if (m_moveTime_ms > 0) {
        // a fixed time per move, use all of it.
        m_hardLimit_ms = minusOverhead(m_moveTime_ms);
        m_softLimit_ms = m_hardLimit_ms;
        return;
    }

    const u64 movesToGo = m_movesToGo > 0 ? m_movesToGo : c_defaultMovesToGo;
    const u64 target = m_timeLeft_ms / movesToGo + (m_increment_ms * 3) / 4;

    // the hard limit lets an iteration started just before the soft limit finish, but no single move
    // may take most of the clock.
    const u64 maxUsage = std::max<u64>(minusOverhead(m_timeLeft_ms) * c_maxClockUsage_pct / 100, 1);
    m_hardLimit_ms = std::min(target * c_hardLimitFactor, maxUsage);
    m_softLimit_ms = std::min(target * c_softLimit_pct / 100, m_hardLimit_ms);
}

void TimeManager::setMoveOverhead(u32 overhead_ms) {
    m_moveOverhead_ms = std::min(overhead_ms, c_maxMoveOverhead_ms);
}

bool TimeManager::continueIterativeDeepening() const {
    if (m_isTimeManaged == false) {
        // If not time-managed (e.g., depth-limited), always allow.
        // The iterative deepener loop will handle the depth limit.
        return true;
    }

    // only start another iteration while within the soft limit, the hard limit aborts it if it runs long.
    return now() < m_softLimit_ms;
}

void TimeManager::begin() {
    m_startTime = chess_time_t::now();    

    if (m_isTimeManaged == false) {
        // For infinite, depth, or nodes search, set stop time to "never"
        m_endTime = timepoint_t::max();
    } 
    else {
        // We have time controls, the hard limit is when the search is aborted.
        calculateLimits();
        m_endTime = m_startTime + ms_t(m_hardLimit_ms);
    }
}

u64 TimeManager::now() const {
    timepoint_t currentTime = chess_time_t::now();
    auto elapsed = std::chrono::duration_cast<ms_t>(currentTime - m_startTime).count();
    return static_cast<u64>(elapsed);
}

u64 TimeManager::softLimit() const {
    return m_isTimeManaged ? m_softLimit_ms : std::numeric_limits<u64>::max();
}

u64 TimeManager::hardLimit() const {
    return m_isTimeManaged ? m_hardLimit_ms : std::numeric_limits<u64>::max();
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
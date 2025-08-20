#include <util/clock.hpp>

void Clock::Start()
{
    m_startTime = std::chrono::high_resolution_clock::now();
    m_running = true;
}

void Clock::Stop()
{
    m_endTime = std::chrono::high_resolution_clock::now();
    m_running = false;
}

i64 Clock::getElapsedTime() const
{
    auto endTime = std::chrono::high_resolution_clock::now();
    if (m_running == false)
        endTime = m_endTime;
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_startTime).count();
}

u64 Clock::calcNodesPerSecond(u64 nodes) const
{
    i64 et = getElapsedTime();
    if (et < 1)
        return 0;
    float seconds = ((float)et / 1000.f);
    
    return (u64)((float)nodes / seconds);
}
#include "clock.hpp"

void Clock::Start()
{
    m_startTime = std::chrono::high_resolution_clock::now();
}

i64 Clock::getElapsedTime() const
{    
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();
}

u64 Clock::calcNodesPerSecond(u64 nodes) const
{
    i64 et = getElapsedTime();
    if (et < 1)
        return 0;
    return nodes / (et / 1000.f);
}
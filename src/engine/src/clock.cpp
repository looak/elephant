#include "clock.hpp"

void Clock::Start()
{
    m_startTime = std::chrono::steady_clock::now();
}

i64 Clock::getElapsedTime() const
{    
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();
}
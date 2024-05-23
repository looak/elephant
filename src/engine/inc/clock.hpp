#pragma once

#include <chrono>

#include "defines.h"


class Clock
{
public:
    Clock() = default;

    void Start();
    void Stop();

    /**
     * @return elapsed time since Start() in milliseconds represented as a signed 64-bit integer.   */
    i64 getElapsedTime() const;

    u64 calcNodesPerSecond(u64 nodes) const;

private:
    bool m_running = false;
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_endTime;

};
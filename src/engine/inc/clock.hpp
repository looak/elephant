#pragma once

#include <chrono>

#include "defines.h"


class Clock
{
public:
    Clock() = default;

    void Start();

    /**
     * @return elapsed time since Start() in milliseconds represented as a signed 64-bit integer.   */
    i64 getElapsedTime() const;

private:
    std::chrono::milliseconds m_elapsedTime;
    std::chrono::steady_clock::time_point m_startTime;
};
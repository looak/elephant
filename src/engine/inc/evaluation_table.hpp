#pragma once
#include "defines.hpp"
#include "move.h"

#include <unordered_map>


struct EvaluationEntry
{    
    i32 score;
};

using EvaluationTable = std::unordered_map<uint64_t, EvaluationEntry>;
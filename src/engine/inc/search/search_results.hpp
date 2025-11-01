#pragma once
#include <move/move.hpp>

/**
 * @brief Represents the result of a search.   */
struct SearchResult {
    i32 score;
    PackedMove move;
    bool ForcedMate = false;
    u64 count = 0;

    void operator=(const SearchResult& other) {
        this->score = other.score;
        this->move = other.move;
        this->ForcedMate = other.ForcedMate;
        this->count = other.count;
    }
};
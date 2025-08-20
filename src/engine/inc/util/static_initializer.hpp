#pragma once

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/rays/rays.hpp>
#include <io/weight_store.hpp>
#include <position/hash_zobrist.hpp>

namespace static_initializer {
bool initialize() {
    attacks::internals::initialize();
    ray::internals::initialize();
    zobrist::internals::initialize();

    assert(zobrist::internals::initialized());

    WeightStore::get()->initialize();
    return true;
}

} // namespace static_initializer
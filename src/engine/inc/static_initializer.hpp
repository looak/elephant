#pragma once

#include "attacks/attacks.hpp"
#include "rays/rays.hpp"
#include "utils/weight_store.hpp"

namespace static_initializer {
bool initialize() {
    attacks::internals::initialize();
    ray::internals::initialize();

    WeightStore::get()->initialize();
    return true;
}

} // namespace static_initializer
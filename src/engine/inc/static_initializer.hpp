#pragma once

#include "attacks/attacks.hpp"
#include "rays/rays.hpp"

namespace static_initializer {
bool initialize() {
    attacks::internals::initialize();
    ray::internals::initialize();
    return true;
}

} // namespace static_initializer
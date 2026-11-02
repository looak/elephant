#pragma once
#include <core/game.hpp>
#include <search/monad.hpp>

// What the CLI's commands work on, kept from one command to the next: searches carry over what they learned until a new
// game. Normal mode's own, uci mode has a Game & a Monad of its own.
struct CommandContext {
    CommandContext() { monad.setHash(c_hashMb); }

    // normal mode has no Hash option, this is what it's always had: more than UCI's default, someone at a prompt asks for
    // deep searches.
    static constexpr size_t c_hashMb = 64;

    Game game;
    Monad monad;
};

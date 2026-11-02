#pragma once
#include <material/chess_piece_defines.hpp>
#include <move/move.hpp>
#include <position/position.hpp>

#include <vector>

class GameContext;

// What a search is allowed to spend and where it may look, the go command's arguments.
struct SearchLimits {
    // search depth in half moves, a.k.a. ply or plies.
    // 0 = no depth limit, the search runs until stopped by time or reaches the ply limit.
    u8 SearchDepth = 0;

    // total amount of time allowed to search for a move in milliseconds.
    // 0 = no time limit
    u32 MoveTime = 0;

    // time limit for white and black in milliseconds including increments.
    // 0 = no time limit.
    u32 WhiteTimelimit = 0;
    u32 BlackTimelimit = 0;
    u32 WhiteTimeIncrement = 0;
    u32 BlackTimeIncrement = 0;

    u32 MovesToGo = 0;
    bool Infinite = false;

    // stop once a search thread has visited this many nodes (nodes + qnodes), 0 = no node limit. The first iteration
    // always completes, so the count can overshoot when the limit is smaller than a depth 1 search.
    u64 NodeLimit = 0;

    // restrict the root to these moves, empty = all legal moves. Only source, target & promotion are compared, moves
    // that aren't legal in the root position are dropped and if none are left the restriction is ignored.
    std::vector<PackedMove> SearchMoves;
};

// The whole question a search answers, a copy. Nothing a search thread reads about the game is shared with whoever owns
// the game, so a position command can arrive at any moment. Every thread of a search reads the same one, nothing in it
// changes while they do.
struct SearchRequest {
    Position root;
    Set toPlay = Set::WHITE;
    // the fifty move rule counter at the root.
    u16 rootHalfmoveClock = 0;
    // the positions before each move of the game, oldest first, the ones the search can repeat besides its own line.
    std::vector<u64> gameHashes;
    // limits.SearchMoves filtered to the moves legal at the root, the only ones searched there. Empty = all moves.
    std::vector<PackedMove> rootMoves;
    SearchLimits limits;
};

// A request for the game's current position.
[[nodiscard]] SearchRequest makeSearchRequest(const GameContext& game, SearchLimits limits);

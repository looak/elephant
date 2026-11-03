#pragma once
#include <memory>

#include <eval/pesto_accumulator.hpp>
#include <position/position_accessors.hpp>
#include <search/search_results.hpp>
#include <system/platform.hpp>

class Chessboard;

struct QuiescenceResult {
    i16 score = 0;
    // the line quiescence plays, empty when it would rather stand pat.
    PVLine line;
    // a mate for either side: the line ends in a mated position, there's no quiet one at its end.
    bool decided = false;
};

// Quiescence search of one position after another, for the tools that want to know what's hanging and have no use for a
// search: the tuner fits on the quiet position at the end of the line, the indexer keeps positions with no line at all.
// Runs to its own end, nothing stops it. Its move ordering and pawn hash tables are kept from one position to the next,
// so the same positions in the same order give the same lines. One thread each.
class QuiescenceProbe {
public:
    QuiescenceProbe();
    ~QuiescenceProbe();

    // Searches board's position, the probe's position stays on it, every move is taken back.
    [[nodiscard]] QuiescenceResult search(const Chessboard& board);

    // Plays line, from the last search, on the probe's position & accumulator.
    void playLine(const PVLine& line);

    [[nodiscard]] PositionReader position() const;
    [[nodiscard]] const PestoAccumulator& accumulator() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

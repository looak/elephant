#pragma once
#include <position/position_accessors.hpp>

class Chessboard;

namespace printer {

void board(std::ostream& output, const Chessboard& board);
void position(std::ostream& output, PositionReader reader);

}  // namespace printer
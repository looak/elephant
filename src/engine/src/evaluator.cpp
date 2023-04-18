#include "evaluator.h"

#include "chessboard.h"
#include "evaluator_data.h"

u64 Evaluator::Evaluate(const Chessboard& board) const
{
    init_tables();
    return eval(board);
}
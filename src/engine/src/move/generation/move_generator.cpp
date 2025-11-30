#include <bitboard/attacks/attacks.hpp>
#include <core/game_context.hpp>
#include <move/move.hpp>
#include <move/generation/move_generator.hpp>
#include <move/generation/move_bulk_generator.hpp>
#include <math/cast.hpp>

template<Set us>
MoveGenerator<us>::MoveGenerator(PositionReader position, MoveGenParams& params) :
m_pinThreats(toSquare(position.material().king<us>().lsbIndex()), position),
    m_position(position),   
    m_currentMoveIndx(0),
    m_moveCount(0),
    m_movesGenerated(false),
    m_stage(Stage::PV_MOVE),
    m_params(params)
{
    if (params.ordering == nullptr || params.ordering->pvMove == PackedMove::NullMove()) {
        m_stage = Stage::CAPTURES;
    }
}

template MoveGenerator<Set::WHITE>::MoveGenerator(PositionReader, MoveGenParams&);
template MoveGenerator<Set::BLACK>::MoveGenerator(PositionReader, MoveGenParams&);

template<Set us>
PrioritizedMove MoveGenerator<us>::pop() {
    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx++];
    }

    if (m_movesGenerated)
        return { PackedMove::NullMove(), 0 };

    PrioritizedMove result = internalGenerateMoves();
    m_currentMoveIndx++;
    return result;
}

template PrioritizedMove MoveGenerator<Set::WHITE>::pop();
template PrioritizedMove MoveGenerator<Set::BLACK>::pop();

template<Set us>
PackedMove MoveGenerator<us>::peek() {
    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx].move;
    }

    if (m_movesGenerated)
        return PackedMove::NullMove();

    return internalGenerateMoves().move;
}

template PackedMove MoveGenerator<Set::WHITE>::peek();
template PackedMove MoveGenerator<Set::BLACK>::peek();

#ifdef DEVELOPMENT_BUILD
template<Set us>
std::vector<PrioritizedMove> MoveGenerator<us>::moves() {
    if (!m_movesGenerated) {
        internalGenerateMovesOrdered();
    }

    std::vector<PrioritizedMove> allMoves;
    allMoves.reserve(m_moveCount);

    for (u32 i = 0; i < m_moveCount; ++i) {
        allMoves.push_back(m_movesBuffer[i]);
    }

    return allMoves;
}

template std::vector<PrioritizedMove> MoveGenerator<Set::WHITE>::moves();
template std::vector<PrioritizedMove> MoveGenerator<Set::BLACK>::moves();

#endif

template<Set us>
PrioritizedMove MoveGenerator<us>::internalGenerateMoves() {    
    if (m_position.material().combine<us>().empty()) {
        // TODO: This should never happen, there will always be a king?
        m_movesGenerated = true;        
    }

    BulkMoveGenerator bulkMoveGen(m_position);

    if (m_movesGenerated == false) {
        // special case for when king is in a double check, no need to generate other moves than king.
        if (m_pinThreats.isCheckedCount() > 1) {
            internalGenerateKingMoves(bulkMoveGen);
        }
        else {
            internalGeneratePawnMoves(bulkMoveGen);
            internalGenerateMovesGeneric<knightId>(bulkMoveGen);
            internalGenerateMovesGeneric<bishopId>(bulkMoveGen);
            internalGenerateMovesGeneric<rookId>(bulkMoveGen);
            internalGenerateMovesGeneric<queenId>(bulkMoveGen);
            internalGenerateKingMoves(bulkMoveGen);
        }

        sortMoves();
        m_movesGenerated = true;
    }

    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx];
    }

    return { PackedMove::NullMove(), 0 };
}

template PrioritizedMove MoveGenerator<Set::WHITE>::internalGenerateMoves();
template PrioritizedMove MoveGenerator<Set::BLACK>::internalGenerateMoves();

template<Set us>
void MoveGenerator<us>::internalGenerateMovesOrdered()
{
    using generatorFunc = std::function<void(MoveGenerator<us>*, BulkMoveGenerator&)>;
    
    static const std::array<generatorFunc, 6> generators = {
        [](MoveGenerator<us>* gen, BulkMoveGenerator& bulkGen) {
            gen->internalGeneratePawnMoves(bulkGen);
        },
        [](MoveGenerator<us>* gen, BulkMoveGenerator& bulkGen) {
            gen->internalGenerateMovesGeneric<knightId>(bulkGen);
        },
        [](MoveGenerator<us>* gen, BulkMoveGenerator& bulkGen) {
            gen->internalGenerateMovesGeneric<bishopId>(bulkGen);
        },
        [](MoveGenerator<us>* gen, BulkMoveGenerator& bulkGen) {
            gen->internalGenerateMovesGeneric<rookId>(bulkGen);
        },
        [](MoveGenerator<us>* gen, BulkMoveGenerator& bulkGen) {
            gen->internalGenerateMovesGeneric<queenId>(bulkGen);
        },
        [](MoveGenerator<us>* gen, BulkMoveGenerator& bulkGen) {
            gen->internalGenerateKingMoves(bulkGen);
        }
    };

    if (m_params.pieceIdFlag == 0) {
        return; // nothing to do
    }

    u64 tmp = m_params.pieceIdFlag;
    u32 piece = intrinsics::lsbIndex(tmp);
    tmp = intrinsics::resetLsb(tmp);
    m_params.pieceIdFlag = static_cast<u8>(tmp);

    BulkMoveGenerator bulkMoveGen(m_position);
    generators[piece](this, bulkMoveGen);

    // sortMoves();
    // m_movesGenerated = true;
}

template void MoveGenerator<Set::WHITE>::internalGenerateMovesOrdered();
template void MoveGenerator<Set::BLACK>::internalGenerateMovesOrdered();

template<Set us>
void MoveGenerator<us>::sortMoves() {
    if (m_params.ordering != nullptr) {
        if (m_params.ordering->pvMove != PackedMove::NullMove()) {
            auto itrMv = std::find_if(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, [&](const PrioritizedMove& pm) {
                return pm.move == m_params.ordering->pvMove;
                });

            if (itrMv != m_movesBuffer.end()) 
                itrMv->priority += move_generator_constants::pvMovePriority;
            
            auto ttMove = std::find_if(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, [&](const PrioritizedMove& pm) {
                return pm.move == m_params.ordering->ttMove;
                });
            
            if (ttMove != m_movesBuffer.end() && ttMove != itrMv)
                ttMove->priority += move_generator_constants::ttMovePriority;
        }
        for (int i = 0; i < 2; ++i) {
            if (m_params.ordering->killers[i] != PackedMove::NullMove()) {
                auto killerMove = std::find_if(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, [&](const PrioritizedMove& pm) {
                    return pm.move == m_params.ordering->killers[i];
                    });
                
                if (killerMove != m_movesBuffer.end()) {
                    killerMove->priority += move_generator_constants::killerMovePriority;
         //           killerMove->priority += m_params.ordering->getHistoryScore(us, killerMove->move.sourceSqr(), killerMove->move.targetSqr());
                }
            }
        }
    }

    // if (m_search != nullptr) {
    //     for (u32 i = 0; i < m_moveCount; ++i) {
    //         auto& move = m_movesBuffer[i];
    //         if (m_search->isKillerMove(move.move, m_ply)) {
    //             move.priority += move_generator_constants::killerMovePriority;
    //             move.priority += m_search->getHistoryHeuristic(static_cast<u8>(m_toMove), move.move.source(), move.move.target());
    //         }
    //     }
    // }    

    PrioritizedMoveComparator comparator;
    std::sort(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, comparator);
}

template<Set us>
void MoveGenerator<us>::internalBuildPawnPromotionMoves(PackedMove move, u32 dstIndex) {

    size_t dstIndex_t = checked_cast<size_t>(dstIndex);
    bool orthogonallyChecked = !(m_pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstIndex_t]).empty();
    bool diagonallyChecked = !(m_pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstIndex_t]).empty();

    std::optional<i16> captureScore = m_position.material().computeCaptureScore<us>(Bitboard(squareMaskTable[dstIndex_t]), pawnId);
    u16 actualCaptureValue = captureScore.has_value() ? move_generator_constants::capturePriority + static_cast<u16>(captureScore.value()) : 0;
    const u16 promotionPriorityValue = move_generator_constants::promotionPriority + actualCaptureValue;

    move.setPromoteTo(queenId);
    PrioritizedMove queenPromote(move, 0);
    queenPromote.setCheck(orthogonallyChecked || diagonallyChecked);
    queenPromote.priority = promotionPriorityValue;

    m_movesBuffer[m_moveCount] = queenPromote;
    m_moveCount++;

    move.setPromoteTo(rookId);
    PrioritizedMove rookPromote(move, 1);
    rookPromote.setCheck(orthogonallyChecked);
    rookPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = rookPromote;
    m_moveCount++;

    move.setPromoteTo(bishopId);
    PrioritizedMove bishopPromote(move, 1);
    bishopPromote.setCheck(diagonallyChecked);
    bishopPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = bishopPromote;
    m_moveCount++;

    // don't I need to check for check here?
    move.setPromoteTo(knightId);
    PrioritizedMove knigthPromote(move, 1);
    knigthPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = knigthPromote;
    m_moveCount++;
}

template void MoveGenerator<Set::WHITE>::internalBuildPawnPromotionMoves(PackedMove, u32);
template void MoveGenerator<Set::BLACK>::internalBuildPawnPromotionMoves(PackedMove, u32);

template<Set us>
void MoveGenerator<us>::internalGeneratePawnMoves(BulkMoveGenerator bulkMoveGen)
{
    u8 usIndx = static_cast<u8>(us);
    const Bitboard movesbb = internalCallBulkGeneratorForPiece(pawnId, bulkMoveGen);
    if (movesbb.empty())
        return;
        
    PieceIsolator<us, pawnId> isolator(m_position, movesbb, m_pinThreats);

    // cache pawns in local variable which we'll use to iterate over all pawns.
    Bitboard pawns = m_position.material().pawns<us>();

    while (pawns.empty() == false) {
        // build source square and remove pawn from pawns bitboard.
        const Square srcSqr = toSquare(pawns.popLsb());
        const u64 promotionMask = pawn_constants::promotionRank[usIndx];

        auto isolated = isolator.isolate(srcSqr);
        while (isolated.captures.empty() == false) {
            u32 dstIndex = isolated.captures.popLsb();
            Square dstSquare = toSquare(dstIndex);

            PrioritizedMove prioratizedMove;
            PackedMove& move = prioratizedMove.move;
            move.setSource(srcSqr);
            move.setTarget(dstSquare);

            // if we're capturing enpassant set the enpassant flag.
            if (m_position.enPassant().readSquare() == dstSquare)
            {
                move.setEnPassant(true);  // sets both capture & enpassant
                dstSquare = m_position.enPassant().readTarget();
            }
            else
                move.setCapture(true);

            auto captureScore = m_position.material().computeCaptureScore<us>(Bitboard(squareMaskTable[*dstSquare]), pawnId);
            prioratizedMove.priority = move_generator_constants::capturePriority + 
                (captureScore.value() * move_generator_constants::mvvLvaMultiplier);

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[checked_cast<size_t>(dstIndex)]) {
                internalBuildPawnPromotionMoves(move, dstIndex);
            }
            else {
                Position checkedPos;
                checkedPos.edit().placePiece(ChessPiece(us, PieceType::PAWN), static_cast<Square>(dstIndex));
                Bitboard threat = checkedPos.read().material().topology<us>().computeThreatenedSquaresPawnBulk();
                if (threat & m_position.material().king<opposing_set<us>()>()) {
                    prioratizedMove.setCheck(true);
                    prioratizedMove.priority += move_generator_constants::checkPriority;
                }

                m_movesBuffer[m_moveCount] = prioratizedMove;
                m_moveCount++;
            }
        }
        while (isolated.quiets.empty() == false) {
            u32 dstIndex = isolated.quiets.popLsb();

            PrioritizedMove prioratizedMove;
            PackedMove& move = prioratizedMove.move;
            move.setSource(srcSqr);
            move.setTarget(toSquare(dstIndex));

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[checked_cast<size_t>(dstIndex)]) {
                internalBuildPawnPromotionMoves(move, dstIndex);
            }
            else {
                Position checkedPos;
                checkedPos.edit().placePiece(ChessPiece(us, PieceType::PAWN), static_cast<Square>(dstIndex));
                Bitboard threat = checkedPos.read().material().topology<us>().computeThreatenedSquaresPawnBulk();
                if (threat & m_position.material().king<opposing_set<us>()>()) {
                    prioratizedMove.setCheck(true);
                    prioratizedMove.priority += move_generator_constants::checkPriority;
                }

                m_movesBuffer[m_moveCount] = prioratizedMove;
                m_moveCount++;
            }
        }
    }
}

template void MoveGenerator<Set::WHITE>::internalGeneratePawnMoves(BulkMoveGenerator);
template void MoveGenerator<Set::BLACK>::internalGeneratePawnMoves(BulkMoveGenerator);

template<Set us>
void MoveGenerator<us>::internalGenerateKingMoves(BulkMoveGenerator bulkMoveGen)
{
    const Bitboard opMaterial = m_position.material().combine<opposing_set<us>()>();
    const u8 setId = static_cast<u8>(us);

    Bitboard movesbb = internalCallBulkGeneratorForPiece(kingId, bulkMoveGen);
#if defined EG_DEBUGGING || defined EG_TESTING
    // during testing and debugging king can be missing
    if (movesbb.empty())
        return;
#endif

    u32 srcIndex = m_position.material().king<us>().lsbIndex();
    u8 castlingRaw = m_position.castling().read() >> (setId * 2);

    while (movesbb.empty() == false) {
        u32 dstIndex = movesbb.popLsb();

        PrioritizedMove prioratizedMove;
        PackedMove& move = prioratizedMove.move;
        move.setSource(toSquare(srcIndex));
        move.setTarget(toSquare(dstIndex));
        Bitboard dstIndexMsk(squareMaskTable[checked_cast<size_t>(dstIndex)]);

        auto captureScore = m_position.material().computeCaptureScore<us>(dstIndexMsk, kingId);
        if (captureScore.has_value()) {
            move.setCapture(true);
            prioratizedMove.priority = move_generator_constants::capturePriority + 
                (captureScore.value() * move_generator_constants::mvvLvaMultiplier);
        }

        if (castlingRaw & 2) {
            u64 queenSideCastleSqrMask = king_constants::queenSideCastleMask & board_constants::baseRankRelative[setId];
            if (dstIndexMsk & queenSideCastleSqrMask) {
                move.setCastleQueenSide(true);
            }
        }
        if (castlingRaw & 1) {
            u64 kingSideCastleSqrMask = king_constants::kingSideCastleMask & board_constants::baseRankRelative[setId];
            if (dstIndexMsk & kingSideCastleSqrMask) {
                move.setCastleKingSide(true);
            }
        }

        m_movesBuffer[m_moveCount] = prioratizedMove;
        m_moveCount++;
    }
}

template void MoveGenerator<Set::WHITE>::internalGenerateKingMoves(BulkMoveGenerator);
template void MoveGenerator<Set::BLACK>::internalGenerateKingMoves(BulkMoveGenerator);

template<Set us>
void MoveGenerator<us>::buildPackedMoveFromBitboard(u8 pieceId, Bitboard movesbb, Square srcSqr, bool capture)
{
    while (movesbb.empty() == false) {
        Square dstSquare = toSquare(movesbb.popLsb());

        PrioritizedMove prioratizedMove;
        PackedMove& move = prioratizedMove.move;
        move.setSource(srcSqr);
        move.setTarget(dstSquare);
        move.setCapture(capture);
        prioratizedMove.priority = 0;

        if (capture) {
            auto captureScore = m_position.material().computeCaptureScore<us>(Bitboard(squareMaskTable[*dstSquare]), pieceId);
            prioratizedMove.priority = move_generator_constants::capturePriority + 
                (captureScore.value() * move_generator_constants::mvvLvaMultiplier);
        }

        // figure out if we're checking the king.
        if (pieceId == rookId || pieceId == queenId) {
            if (m_pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[*dstSquare]) {
                prioratizedMove.setCheck(true);
                prioratizedMove.priority += move_generator_constants::checkPriority;
            }
        }
        else if (pieceId == bishopId || pieceId == queenId) {
            if (m_pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[*dstSquare]) {
                prioratizedMove.setCheck(true);
                prioratizedMove.priority += move_generator_constants::checkPriority;
            }
        }
        m_movesBuffer[m_moveCount] = prioratizedMove;
        m_moveCount++;
    }
}

template void MoveGenerator<Set::WHITE>::buildPackedMoveFromBitboard(u8, Bitboard, Square, bool);
template void MoveGenerator<Set::BLACK>::buildPackedMoveFromBitboard(u8, Bitboard, Square, bool);

template<Set us>
Bitboard MoveGenerator<us>::internalCallBulkGeneratorForPiece(u8 pieceId, BulkMoveGenerator bulkMoveGen)
{
    // TODO: Next major refactor should figure out a smother way to setup this, MoveGenerator should probably be templated 
    // on the filter as well.
    // TODO: BulkMoveGen should just generate all moves and then the move generator should filter them. This to allow
    // me to generate checking moves in quiescence search which currently ignores them.
    if (m_params.moveFilter == MoveTypes::ALL) {
        return bulkMoveGen.computeBulkMovesGeneric<us, MoveTypes::ALL>(pieceId);
    }
    else if (m_params.moveFilter == MoveTypes::CAPTURES_ONLY) {
        return bulkMoveGen.computeBulkMovesGeneric<us, MoveTypes::CAPTURES_ONLY>(pieceId);
    }
    /*else {
        return bulkMoveGen.computeBulkMovesGeneric<us, pieceId, MoveTypes::ALL>();
    }*/

    return 0;
}
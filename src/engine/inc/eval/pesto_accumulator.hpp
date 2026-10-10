// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2025  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.

/**
 * @file pesto_accumulator.hpp
 * @brief PestoAccumulator, PeSTO's midgame & endgame sums, game phase and piece_constants material kept up to date
 * as pieces are added & removed, so the evaluator doesn't walk every piece on every call.
 *
 * Updated through PositionProxy::pieceAdded / pieceRemoved, the one place a piece entering or leaving a square is
 * reported. computeFromScratch gives the same values from the bitboards, used to initialize and to verify.
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <eval/pesto_tables.hpp>
#include <material/chess_piece.hpp>
#include <material/material_mask.hpp>

struct PestoAccumulator {
    i32 midgame = 0;   // white - black, material & piece square tables
    i32 endgame = 0;
    i32 phase = 0;     // uncapped, promotions can push it past maxGamePhase
    i32 material = 0;  // white - black, piece_constants::value

    void add(ChessPiece piece, Square sqr) { apply(piece, sqr, 1); }
    void remove(ChessPiece piece, Square sqr) { apply(piece, sqr, -1); }

    // tapered PeSTO score from white's perspective.
    [[nodiscard]] i32 taperedScore() const {
        const i32 cappedPhase = phase < evaluator_data::maxGamePhase ? phase : evaluator_data::maxGamePhase;
        return (midgame * cappedPhase + endgame * (evaluator_data::maxGamePhase - cappedPhase)) / evaluator_data::maxGamePhase;
    }

    [[nodiscard]] static PestoAccumulator computeFromScratch(const MaterialPositionMask& material) {
        PestoAccumulator result;
        for (u8 set = 0; set < 2; ++set) {
            for (u8 pieceId = 0; pieceId < 6; ++pieceId) {
                const ChessPiece piece(static_cast<Set>(set), static_cast<PieceType>(pieceId + 1));
                Bitboard pieces = material.read(static_cast<Set>(set), pieceId);
                while (pieces.empty() == false)
                    result.add(piece, static_cast<Square>(pieces.popLsb()));
            }
        }
        return result;
    }

    bool operator==(const PestoAccumulator&) const = default;

private:
    void apply(ChessPiece piece, Square sqr, i32 sign) {
        const u32 pieceId = piece.index();
        // tables have A8 at index 0, see evaluator_data.
        const u32 tableSqr = piece.isWhite() ? evaluator_data::flip(static_cast<u32>(sqr)) : static_cast<u32>(sqr);
        const i32 setSign = piece.isWhite() ? sign : -sign;

        midgame += setSign * (evaluator_data::pestoMaterial_mg[pieceId] + evaluator_data::pestoTables_mg[pieceId][tableSqr]);
        endgame += setSign * (evaluator_data::pestoMaterial_eg[pieceId] + evaluator_data::pestoTables_eg[pieceId][tableSqr]);
        phase += sign * evaluator_data::gamePhaseIncrement[pieceId];
        material += setSign * static_cast<i32>(piece_constants::value[pieceId]);
    }
};

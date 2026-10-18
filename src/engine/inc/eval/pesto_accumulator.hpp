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
 * @brief PestoAccumulator, PeSTO's packed midgame & endgame sum, game phase, piece_constants material and the piece
 * totals the endgame coefficient uses, kept up to date as pieces are added & removed so the evaluator doesn't walk
 * every piece on every call.
 *
 * Lives outside the position, search owns one (ThreadSearchContext::accumulator) and a MoveExecutor given it adds &
 * removes pieces as moves are made and unmade. computeFromScratch gives the same values from the bitboards, used to
 * initialize and to verify.
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <core/square.hpp>
#include <eval/pesto_tables.hpp>
#include <material/chess_piece.hpp>

// What computeFromScratch reports to a trace for every piece: its piece index (pawn 0 to king 5), the square it reads
// the tables at (A8 = 0, see pesto_tables.hpp) and its sign, +1 white & -1 black. The PeSTO score is the sum of
// sign * (pestoMaterial[pieceId] + pestoTables[pieceId][tableSqr]) over every piece, which is what the tuner fits.
struct NoPestoTrace {
    constexpr void piece(u32, u32, i32) {}
};

struct PestoAccumulator {
    TaperedScore psqt = S(0, 0);  // white - black, material & piece square tables, midgame & endgame packed
    i32 phase = 0;     // uncapped, promotions can push it past maxGamePhase, taper caps it
    i32 material = 0;  // white - black, piece_constants::value
    i32 pieceCount = 0;       // every piece on the board, kings included
    i32 nonKingMaterial = 0;  // white + black, piece_constants::value of everything but the kings

    void add(ChessPiece piece, Square sqr) { NoPestoTrace none; apply(piece, sqr, 1, none); }
    void remove(ChessPiece piece, Square sqr) { NoPestoTrace none; apply(piece, sqr, -1, none); }

    // tapered PeSTO score from white's perspective.
    [[nodiscard]] i32 taperedScore() const { return evaluator_data::taper(psqt, phase); }

    // Material is a MaterialPositionMask, templated so this header doesn't need it.
    template<typename Material>
    [[nodiscard]] static PestoAccumulator computeFromScratch(const Material& material) {
        NoPestoTrace none;
        return computeFromScratch(material, none);
    }

    // The same, telling trace every piece's terms, see NoPestoTrace. The tuner's trace records them.
    template<typename Material, typename Trace>
    [[nodiscard]] static PestoAccumulator computeFromScratch(const Material& material, Trace& trace) {
        PestoAccumulator result;
        for (u8 set = 0; set < 2; ++set) {
            for (u8 pieceId = 0; pieceId < 6; ++pieceId) {
                const ChessPiece piece(static_cast<Set>(set), static_cast<PieceType>(pieceId + 1));
                auto pieces = material.read(static_cast<Set>(set), pieceId);
                while (pieces.empty() == false)
                    result.apply(piece, static_cast<Square>(pieces.popLsb()), 1, trace);
            }
        }
        return result;
    }

    bool operator==(const PestoAccumulator&) const = default;

private:
    template<typename Trace>
    void apply(ChessPiece piece, Square sqr, i32 sign, Trace& trace) {
        const u32 pieceId = piece.index();
        // tables have A8 at index 0, see evaluator_data.
        const u32 tableSqr = piece.isWhite() ? evaluator_data::flip(static_cast<u32>(sqr)) : static_cast<u32>(sqr);
        const i32 setSign = piece.isWhite() ? sign : -sign;

        psqt += setSign * (evaluator_data::pestoMaterial[pieceId] + evaluator_data::pestoTables[pieceId][tableSqr]);
        trace.piece(pieceId, tableSqr, setSign);
        phase += sign * evaluator_data::gamePhaseIncrement[pieceId];
        material += setSign * static_cast<i32>(piece_constants::value[pieceId]);
        pieceCount += sign;
        if (pieceId != kingId)
            nonKingMaterial += sign * static_cast<i32>(piece_constants::value[pieceId]);
    }
};

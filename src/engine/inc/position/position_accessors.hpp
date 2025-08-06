#pragma once
#include <position/position_proxy.hpp>

struct PositionEditPolicy {
    using position_t = Position&;
    using castling_t = CastlingStateInfo&;
    using en_passant_t = EnPassantStateInfo&;
    using material_t = MaterialPositionMask&;
    using hash_t = u64&;
    
    //  currently we don't have a mutable chess piece type on the position proxy.
    // TODO: implement a way to modify the chess piece on a poistion proxy, would only allow to set or unset a piece
    // since the material is stored in two bitboards, one for the set and one for the piece type.
    using chess_piece_t = const ChessPiece;
};

struct PositionReadOnlyPolicy {
    using position_t = const Position&;
    using castling_t = const CastlingStateInfo;
    using en_passant_t = const EnPassantStateInfo;
    using material_t = const MaterialPositionMask&;
    using hash_t = const u64;

    using chess_piece_t = const ChessPiece;
};

typedef PositionProxy<PositionEditPolicy> PositionEditor;
typedef PositionProxy<PositionReadOnlyPolicy> PositionReader;
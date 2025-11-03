#include <position/position_accessors.hpp>
#include <position/position.hpp>

namespace ElephantTest {
namespace chess_positions {


void defaultStartingPosition(PositionEditor position) {

    auto K = WHITEKING;
    auto Q = WHITEQUEEN;
    auto B = WHITEBISHOP;
    auto N = WHITEKNIGHT;
    auto R = WHITEROOK;
    auto P = WHITEPAWN;

    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;

    position.placePiece(R, Square::A1);
    position.placePiece(N, Square::B1);
    position.placePiece(B, Square::C1);
    position.placePiece(Q, Square::D1);
    position.placePiece(K, Square::E1);
    position.placePiece(B, Square::F1);
    position.placePiece(N, Square::G1);
    position.placePiece(R, Square::H1);

    position.placePiece(P, Square::A2);
    position.placePiece(P, Square::B2);
    position.placePiece(P, Square::C2);
    position.placePiece(P, Square::D2);
    position.placePiece(P, Square::E2);
    position.placePiece(P, Square::F2);
    position.placePiece(P, Square::G2);
    position.placePiece(P, Square::H2);

    position.placePiece(r, Square::A8);
    position.placePiece(n, Square::B8);
    position.placePiece(b, Square::C8);
    position.placePiece(q, Square::D8);
    position.placePiece(k, Square::E8);
    position.placePiece(b, Square::F8);
    position.placePiece(n, Square::G8);
    position.placePiece(r, Square::H8);

    position.placePiece(p, Square::A7);
    position.placePiece(p, Square::B7);
    position.placePiece(p, Square::C7);
    position.placePiece(p, Square::D7);
    position.placePiece(p, Square::E7);
    position.placePiece(p, Square::F7);
    position.placePiece(p, Square::G7);
    position.placePiece(p, Square::H7);

    position.castling().grantAll();
    position.enPassant().clear();
}

void windmillPosition(PositionEditor position) {
    auto K = WHITEKING;
    auto Q = WHITEQUEEN;
    auto B = WHITEBISHOP;
    auto N = WHITEKNIGHT;
    auto R = WHITEROOK;
    auto P = WHITEPAWN;

    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;

    position.placePiece(r, Square::A8);
    position.placePiece(r, Square::E8);
    position.placePiece(n, Square::F8);
    position.placePiece(k, Square::G8);

    position.placePiece(p, Square::A7);
    position.placePiece(b, Square::B7);
    position.placePiece(p, Square::F7);
    position.placePiece(p, Square::G7);

    position.placePiece(p, Square::D6);
    position.placePiece(p, Square::E6);
    position.placePiece(p, Square::H6);

    position.placePiece(q, Square::B5);
    position.placePiece(B, Square::G5);
    position.placePiece(Q, Square::H5);

    position.placePiece(P, Square::B4);
    position.placePiece(P, Square::D4);

    position.placePiece(N, Square::E3);
    position.placePiece(R, Square::G3);

    position.placePiece(P, Square::A2);
    position.placePiece(P, Square::F2);
    position.placePiece(P, Square::G2);
    position.placePiece(P, Square::H2);

    position.placePiece(R, Square::E1);
    position.placePiece(K, Square::G1);

}

void nepomniachtchiResignsGameSix(PositionEditor position) {
    position.placePiece(BLACKQUEEN, Square::G1);
    position.placePiece(BLACKKING, Square::D8);
    position.placePiece(WHITEPAWN, Square::E6);
    position.placePiece(WHITEPAWN, Square::F5);
    position.placePiece(WHITEKNIGHT, Square::G7);
    position.placePiece(WHITEROOK, Square::F7);
    position.placePiece(WHITEKING, Square::H4);
}

void move18_FischerSpassky(PositionEditor position) {
    auto K = WHITEKING;
    auto Q = WHITEQUEEN;
    auto B = WHITEBISHOP;
    auto N = WHITEKNIGHT;
    auto R = WHITEROOK;
    auto P = WHITEPAWN;

    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;

    position.placePiece(R, Square::A1);
    position.placePiece(N, Square::B1);
    position.placePiece(Q, Square::D1);
    position.placePiece(R, Square::E1);
    position.placePiece(K, Square::G1);

    position.placePiece(P, Square::A2);
    position.placePiece(P, Square::B2);
    position.placePiece(P, Square::F2);
    position.placePiece(P, Square::G2);

    position.placePiece(B, Square::B3);
    position.placePiece(N, Square::F3);
    position.placePiece(P, Square::H3);

    position.placePiece(p, Square::B4);
    position.placePiece(n, Square::E4);

    position.placePiece(p, Square::C5);
    position.placePiece(P, Square::E5);

    position.placePiece(p, Square::D6);
    position.placePiece(p, Square::H6);

    position.placePiece(b, Square::B7);
    position.placePiece(n, Square::D7);
    position.placePiece(q, Square::E7);
    position.placePiece(p, Square::F7);
    position.placePiece(p, Square::G7);

    position.placePiece(r, Square::A8);
    position.placePiece(r, Square::F8);
    position.placePiece(k, Square::G8);

    position.castling().clear();
}

} // namespace chess_positions
} // namespace ElephantTest

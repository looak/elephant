#include <vector>
#include <string>

struct SearchCase {
    std::string fen;
    std::string expectedMove;
};

const std::vector<SearchCase> s_searchCases = {
    // very tempting rook king fork but actually checking the king with the queen is the better move.
    // { "r3kb1r/1b1n2p1/p3Nn1p/3Pp3/1p4PP/3QBP2/qPP5/2KR1B1R w kq - 0 1", "Qg6+" }
};

const std::vector<SearchCase> s_mateInThree = {
    // The end game Paul Vaitonis and Reuben Fine , Stockholm 1937 ended in a victory for Fine who won in 3 moves.
    { "4r1k1/5bpp/2p5/3pr3/8/1B3pPq/PPR2P2/2R2QK1 b - - 0 1", "e5e1" },
    { "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1", "g3g6" }
};
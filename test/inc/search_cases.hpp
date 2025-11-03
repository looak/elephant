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

const std::vector<SearchCase> s_mateInFive = {
    {"2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w - - 0 1", "e7e8" },
    {"6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - - 0 1", "h4f4" },
    {"6r1/p3p1rk/1p1pPp1p/q3n2R/4P3/3BR2P/PPP2QP1/7K w - - 0 1", "h5h6" }
};

const std::vector<SearchCase> s_mateInEight = {
    {"4r2k/1p3rbp/2p1N1pn/p3n3/P2NB3/1P4q1/4R1P1/B1Q2RK1 b - - 4 32", "h6g4" } 
};
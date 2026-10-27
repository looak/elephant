#include <write_tables.hpp>

#include <format>
#include <stdexcept>

#include <trace.hpp>

namespace tuner {

namespace {

// pestoMaterial's body, one line: "    S(82, 94), S(337, 281), ...".
std::string materialBody(const std::vector<TaperedScore>& values, const std::string& newline) {
    std::string body = "    ";
    for (u32 pieceId = 0; pieceId < 6; ++pieceId) {
        const TaperedScore score = values[params::material(pieceId)];
        body += std::format("{}S({}, {})", pieceId == 0 ? "" : ", ", mg_score(score), eg_score(score));
    }
    return body + newline;
}

// a piece table's body: the file letters, then a rank per line from 8 down to 1, A8 first, as published.
std::string tableBody(const std::vector<TaperedScore>& values, u32 pieceId, const std::string& newline) {
    std::string body = "    //   A             B             C             D             E             F             G"
                       "             H" + newline;
    for (u32 rank = 0; rank < 8; ++rank) {
        body += "    ";
        for (u32 file = 0; file < 8; ++file) {
            const TaperedScore score = values[params::psqt(pieceId, rank * 8 + file)];
            body += std::format("S({:4},{:4}), ", mg_score(score), eg_score(score));
        }
        body += std::format("// {}", 8 - rank) + newline;
    }
    return body;
}

// replaces what's between "<declaration> = {<newline>" and the "};" that closes it.
void replaceBody(std::string& header, const std::string& declaration, const std::string& body,
                 const std::string& newline) {
    const std::string opening = declaration + " = {" + newline;
    const std::size_t start = header.find(opening);
    if (start == std::string::npos)
        throw std::runtime_error("pesto_tables.hpp has no " + declaration);
    const std::size_t bodyStart = start + opening.size();
    const std::size_t bodyEnd = header.find("};", bodyStart);
    if (bodyEnd == std::string::npos)
        throw std::runtime_error(declaration + " isn't closed");
    header.replace(bodyStart, bodyEnd - bodyStart, body);
}

} // namespace

std::string rewriteMobilityTables(const std::string& header, const std::vector<TaperedScore>& values) {
    const std::string newline = header.find("\r\n") != std::string::npos ? "\r\n" : "\n";

    std::string result = header;
    const char* names[6] = { nullptr, "knightMobility", "bishopMobility", "rookMobility", "queenMobility", nullptr };
    for (u32 pieceId = 1; pieceId <= 4; ++pieceId) {
        // eight values to a line: "    S(   0,   0), S(  12,  -3), ...,".
        const u32 counts = params::mobilityCounts[pieceId];
        std::string body;
        for (u32 safe = 0; safe < counts; ++safe) {
            const TaperedScore score = values[params::mobility(pieceId, safe)];
            body += safe % 8 == 0 ? "    " : " ";
            body += std::format("S({:4},{:4}),", mg_score(score), eg_score(score));
            if (safe % 8 == 7 || safe + 1 == counts)
                body += newline;
        }
        replaceBody(result, std::format("constexpr TaperedScore {}[{}]", names[pieceId], counts), body, newline);
    }
    return result;
}

std::string rewritePawnTables(const std::string& header, const std::vector<TaperedScore>& values) {
    const std::string newline = header.find("\r\n") != std::string::npos ? "\r\n" : "\n";

    struct Table {
        const char* name;
        u32 size;
        u32 (*index)(u32);
        u32 first;  // what index() takes for the table's first value: rank 2 is 1, file a is 0
    };
    const Table tables[10] = { { "passedPawn", 6, params::passed, 1 },
                               { "isolatedPawn", 4, params::isolated, 0 },
                               { "doubledPawn", 4, params::doubled, 0 },
                               { "backwardPawn", 4, params::backward, 0 },
                               { "connectedPawn", 6, params::connected, 1 },
                               { "passedOwnKing", 8, params::passedOwnKing, 0 },
                               { "passedEnemyKing", 8, params::passedEnemyKing, 0 },
                               { "passedBlocked", 6, params::passedBlocked, 1 },
                               { "passedFreePath", 6, params::passedFreePath, 1 },
                               { "passedRookBehind", 2, params::passedRookBehind, 0 } };
    std::string result = header;
    for (const Table& table : tables) {
        // one line: "    S(   0,   0), S(  12,  -3), ...,".
        std::string body = "    ";
        for (u32 i = 0; i < table.size; ++i) {
            const TaperedScore score = values[table.index(table.first + i)];
            body += std::format("{}S({:4},{:4}),", i == 0 ? "" : " ", mg_score(score), eg_score(score));
        }
        replaceBody(result, std::format("constexpr TaperedScore {}[{}]", table.name, table.size), body + newline,
                    newline);
    }
    return result;
}

std::string rewriteKingTables(const std::string& header, const std::vector<TaperedScore>& values) {
    const std::string newline = header.find("\r\n") != std::string::npos ? "\r\n" : "\n";

    struct Table {
        const char* name;
        u32 offset;
        u32 size;
    };
    const Table tables[4] = { { "kingShelter", params::kingShelterOffset, 32 },
                              { "kingStorm", params::kingStormOffset, 32 },
                              { "kingStormBlocked", params::kingStormBlockedOffset, 8 },
                              { "kingDanger", params::kingDangerOffset, 64 } };
    std::string result = header;
    for (const Table& table : tables) {
        // eight values to a line, a folded file each for shelter & storm, eight units each for danger: "    S(   0,   0), S(  12,  -3), ...,".
        std::string body;
        for (u32 i = 0; i < table.size; ++i) {
            const TaperedScore score = values[table.offset + i];
            body += i % 8 == 0 ? "    " : " ";
            body += std::format("S({:4},{:4}),", mg_score(score), eg_score(score));
            if (i % 8 == 7)
                body += newline;
        }
        replaceBody(result, std::format("constexpr TaperedScore {}[{}]", table.name, table.size), body, newline);
    }
    return result;
}

std::string rewritePieceTables(const std::string& header, const std::vector<TaperedScore>& values) {
    const std::string newline = header.find("\r\n") != std::string::npos ? "\r\n" : "\n";

    // one line: "    S(   0,   0), S(  30,  50), S(   0,   0),".
    std::string body = "    ";
    for (u32 pieceId = 1; pieceId <= 3; ++pieceId) {
        const TaperedScore score = values[params::piecePair(pieceId)];
        body += std::format("{}S({:4},{:4}),", pieceId == 1 ? "" : " ", mg_score(score), eg_score(score));
    }
    std::string result = header;
    replaceBody(result, "constexpr TaperedScore piecePair[3]", body + newline, newline);

    // one line each, undefended then defended.
    const char* names[2] = { "knightOutpost", "bishopOutpost" };
    for (u32 pieceId = 1; pieceId <= 2; ++pieceId) {
        std::string line = "    ";
        for (u32 defended = 0; defended < 2; ++defended) {
            const TaperedScore score = values[params::outpost(pieceId, defended)];
            line += std::format("{}S({:4},{:4}),", defended == 0 ? "" : " ", mg_score(score), eg_score(score));
        }
        replaceBody(result, std::format("constexpr TaperedScore {}[2]", names[pieceId - 1]), line + newline, newline);
    }
    return result;
}

std::string rewritePestoTables(const std::string& header, const std::vector<TaperedScore>& values) {
    // whatever line ending the file has, the rewritten lines get the same.
    const std::string newline = header.find("\r\n") != std::string::npos ? "\r\n" : "\n";

    std::string result = header;
    replaceBody(result, "constexpr TaperedScore pestoMaterial[6]", materialBody(values, newline), newline);

    const char* names[6] = { "pestoPawn", "pestoKnight", "pestoBishop", "pestoRook", "pestoQueen", "pestoKing" };
    for (u32 pieceId = 0; pieceId < 6; ++pieceId) {
        replaceBody(result, std::format("constexpr TaperedScore {}[64]", names[pieceId]),
                    tableBody(values, pieceId, newline), newline);
    }
    return result;
}

} // namespace tuner

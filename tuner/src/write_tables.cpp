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

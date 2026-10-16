#include <dataset.hpp>

#include <cctype>
#include <charconv>
#include <exception>
#include <format>
#include <fstream>
#include <stdexcept>

#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>

namespace tuner {

namespace {

constexpr std::size_t c_maxReportedErrors = 5;

std::vector<std::string_view> splitWhitespace(std::string_view line) {
    std::vector<std::string_view> tokens;
    std::size_t pos = 0;
    while (pos < line.size()) {
        while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
            ++pos;
        const std::size_t start = pos;
        while (pos < line.size() && !std::isspace(static_cast<unsigned char>(line[pos])))
            ++pos;
        if (pos > start)
            tokens.push_back(line.substr(start, pos - start));
    }
    return tokens;
}

bool isCounter(std::string_view token) {
    if (token.empty())
        return false;
    for (char c : token)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
    return true;
}

// "1-0", "1/2-1/2", "0-1", or a number 1, 1.0, 0.5, 0, 0.0. Quotes, brackets & a trailing ; are stripped first.
std::optional<double> parseResult(std::string_view token) {
    while (!token.empty() && (token.back() == ';' || token.back() == '"' || token.back() == ']'))
        token.remove_suffix(1);
    while (!token.empty() && (token.front() == '"' || token.front() == '['))
        token.remove_prefix(1);

    if (token == "1-0")
        return 1.0;
    if (token == "0-1")
        return 0.0;
    if (token == "1/2-1/2")
        return 0.5;

    double value = 0.0;
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || end != token.data() + token.size())
        return std::nullopt;
    if (value == 0.0 || value == 0.5 || value == 1.0)
        return value;
    return std::nullopt;
}

}  // namespace

std::optional<TuningPosition> parseLine(std::string_view line) {
    const std::vector<std::string_view> tokens = splitWhitespace(line);
    if (tokens.size() < 5)
        return std::nullopt;

    // board, side to move, castling & en passant, then the counters when both are there. Checked before the result,
    // a FEN with counters and no result would otherwise read its halfmove clock 0 as a black win.
    std::size_t fenTokens = 4;
    if (tokens.size() >= 6 && isCounter(tokens[4]) && isCounter(tokens[5]))
        fenTokens = 6;

    std::string fen(tokens[0]);
    for (std::size_t i = 1; i < fenTokens; ++i) {
        fen += ' ';
        fen += tokens[i];
    }

    // the first token after the FEN that reads as a result: [1.0], c9 "1-0"; or a bare 1-0.
    for (std::size_t i = fenTokens; i < tokens.size(); ++i) {
        if (const std::optional<double> result = parseResult(tokens[i]))
            return TuningPosition{ std::move(fen), *result };
    }
    return std::nullopt;
}

std::vector<TuningPosition> loadDataset(const std::string& path, LoadReport& report) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error(std::format("Can't open dataset {}", path));

    auto refuse = [&report](std::size_t lineNumber, std::string_view reason) {
        if (report.firstErrors.size() < c_maxReportedErrors)
            report.firstErrors.push_back(std::format("{}: {}", lineNumber, reason));
    };

    std::vector<TuningPosition> positions;
    std::string line;
    while (std::getline(file, line)) {
        ++report.lines;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        const std::size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line[first] == '#') {
            ++report.skipped;
            continue;
        }

        std::optional<TuningPosition> position = parseLine(line);
        if (!position) {
            ++report.noResult;
            refuse(report.lines, "no result found");
            continue;
        }

        bool valid = false;
        try {
            Chessboard board;
            valid = io::fen_parser::deserialize(position->fen, board);
        }
        catch (const std::exception&) {
            valid = false;
        }
        if (!valid) {
            ++report.badFen;
            refuse(report.lines, std::format("invalid FEN '{}'", position->fen));
            continue;
        }

        positions.push_back(std::move(*position));
    }
    return positions;
}

}  // namespace tuner

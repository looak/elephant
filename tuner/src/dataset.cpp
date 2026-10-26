#include <dataset.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <exception>
#include <format>
#include <fstream>
#include <ios>
#include <stdexcept>

#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>

namespace tuner {

namespace {

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

// "1-0", "1/2-1/2", "0-1", or a number from 0 to 1: 1, 1.0, 0.5, 0, 0.0, 0.75. Quotes, brackets & a trailing ; are stripped first.
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
    // 1, 0.5, 0, and any average of results in between: phant-indexer --draw --merge writes [0.75] for a position four
    // games reached, three won by white.
    if (value >= 0.0 && value <= 1.0)
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

void LoadReport::append(const LoadReport& next) {
    for (const LoadError& error : next.firstErrors) {
        if (firstErrors.size() < c_maxErrors)
            firstErrors.push_back({ lines + error.line, error.reason });
    }
    lines += next.lines;
    skipped += next.skipped;
    noResult += next.noResult;
    badFen += next.badFen;
}

namespace {

// binary, so byte offsets are exact whatever the line endings. getline leaves a CRLF file's \r, lines strip it.
std::ifstream openDataset(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error(std::format("Can't open dataset {}", path));
    return file;
}

bool isSkipped(std::string_view line) {
    const std::size_t first = line.find_first_not_of(" \t\r");
    return first == std::string_view::npos || line[first] == '#';
}

// the offset of the line start at or after offset, moving past the rest of a line offset falls inside.
std::uint64_t nextLineStart(std::ifstream& file, std::uint64_t offset, std::uint64_t size) {
    if (offset == 0 || offset >= size)
        return offset >= size ? size : 0;
    file.clear();
    file.seekg(static_cast<std::streamoff>(offset - 1));
    std::string rest;
    std::getline(file, rest);
    return std::min(size, offset - 1 + rest.size() + 1);
}

}  // namespace

std::vector<ByteRange> splitDataset(const std::string& path, std::size_t count, std::size_t limit) {
    std::ifstream file = openDataset(path);
    file.seekg(0, std::ios::end);
    std::uint64_t end = static_cast<std::uint64_t>(file.tellg());
    file.seekg(0);

    // the first limit positions end where the line after the last of them starts.
    if (limit != 0) {
        std::uint64_t offset = 0;
        std::size_t positions = 0;
        std::string line;
        while (positions < limit && std::getline(file, line)) {
            offset += line.size() + 1;
            if (!isSkipped(line))
                ++positions;
        }
        end = std::min(end, offset);
    }

    std::vector<ByteRange> ranges;
    std::uint64_t begin = 0;
    for (std::size_t i = 1; i <= count && begin < end; ++i) {
        const std::uint64_t target = i == count ? end : end * i / count;
        const std::uint64_t split = std::min(end, nextLineStart(file, std::max(target, begin), end));
        if (split > begin) {
            ranges.push_back({ begin, split });
            begin = split;
        }
    }
    return ranges;
}

void forEachPosition(const std::string& path, ByteRange range, LoadReport& report,
                     const std::function<bool(const TuningPosition&)>& handle) {
    std::ifstream file = openDataset(path);
    file.seekg(static_cast<std::streamoff>(range.begin));

    auto refuse = [&report](std::string_view reason) {
        if (report.firstErrors.size() < LoadReport::c_maxErrors)
            report.firstErrors.push_back({ report.lines, std::string(reason) });
    };

    std::uint64_t offset = range.begin;
    std::string line;
    while (offset < range.end && std::getline(file, line)) {
        offset += line.size() + 1;
        ++report.lines;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (isSkipped(line)) {
            ++report.skipped;
            continue;
        }

        const std::optional<TuningPosition> position = parseLine(line);
        if (!position) {
            ++report.noResult;
            refuse("no result found");
            continue;
        }
        if (!handle(*position)) {
            ++report.badFen;
            refuse(std::format("invalid FEN '{}'", position->fen));
        }
    }
}

std::vector<TuningPosition> loadDataset(const std::string& path, LoadReport& report) {
    std::vector<TuningPosition> positions;
    for (const ByteRange& range : splitDataset(path, 1, 0)) {
        forEachPosition(path, range, report, [&positions](const TuningPosition& position) {
            bool valid = false;
            try {
                Chessboard board;
                valid = io::fen_parser::deserialize(position.fen, board);
            }
            catch (const std::exception&) {
                valid = false;
            }
            if (valid)
                positions.push_back(position);
            return valid;
        });
    }
    return positions;
}

}  // namespace tuner

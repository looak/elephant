// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

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
#pragma once

// With OUTPUT_LOG_TO_FILE, main copies std::cout into logs/cout_<time>.log alongside the console.
#include <fstream>
#include <ostream>
#include <streambuf>
#include <string>

namespace logging {
namespace internals {
class DualStreamBuffer : public std::streambuf {
public:
    DualStreamBuffer(std::streambuf* one, std::streambuf* two) :
        m_bufferOne(one),
        m_bufferTwo(two)
    {
    }

protected:
    int sync() override
    {
        // int result1 = m_bufferOne->pubsync();
        // int result2 = m_bufferTwo->pubsync();
        // return (result1 == 0 && result2 == 0) ? 0 : -1;
        return 0;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) override
    {
        m_bufferOne->sputn(s, n);
        m_bufferTwo->sputn(s, n);
        return n;
    }

    int overflow(int c) override
    {
        if (c != EOF) {
            m_bufferOne->sputc(static_cast<char>(c));
            m_bufferTwo->sputc(static_cast<char>(c));
        }
        return c;
    }

private:
    std::streambuf* m_bufferOne;
    std::streambuf* m_bufferTwo;
};
} // namespace internals

class ScopedDualRedirect {
public:
    ScopedDualRedirect(std::ostream& stream, const std::string& filename) :
        m_originalStream(stream),
        m_originalBuffer(stream.rdbuf()),
        m_fileStream(filename, std::ios::app)
    {
        m_dualBuffer = new internals::DualStreamBuffer(m_originalStream.rdbuf(), m_fileStream.rdbuf());
        m_originalStream.rdbuf(m_dualBuffer);
    }
    ~ScopedDualRedirect()
    {
        m_originalStream.rdbuf(m_originalBuffer);
        delete m_dualBuffer;
    }

private:
    std::ostream& m_originalStream;
    std::streambuf* m_originalBuffer;
    std::ofstream m_fileStream;
    internals::DualStreamBuffer* m_dualBuffer;
};
} // namespace logging

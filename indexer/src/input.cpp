#include "input.hpp"

#include <algorithm>
#include <format>

#include <bzlib.h>
#include <zlib.h>

namespace indexer {
namespace {
    // the most either library takes or gives in one call, their counts are 32 bit.
    constexpr size_t c_chunk = size_t(1) << 26;

    // A stream decompressor, bz2's or zlib's, over the whole input. step() runs the library once over the next input
    // & output it's given: 1 at the end of a stream, 0 to go on, -1 on an error; restart() starts the next stream.
    template<typename Stream>
    bool run(std::string_view bytes, std::string& out, std::string& error, Stream& stream)
    {
        size_t in = 0;
        out.clear();
        for (;;) {
            const size_t written = out.size();
            out.resize(written + c_chunk);
            const size_t input = std::min(c_chunk, bytes.size() - in);
            size_t consumed = 0;
            size_t produced = 0;
            int ended = stream.step(bytes.data() + in, input, out.data() + written, c_chunk, consumed, produced, error);
            in += consumed;
            out.resize(written + produced);
            if (ended < 0)
                return false;
            if (ended > 0) {
                if (in == bytes.size())
                    return true;
                if (!stream.restart(error))
                    return false;
                continue;
            }
            if (in == bytes.size() && produced == 0 && consumed == 0) {
                error = "the input ends inside a compressed stream";
                return false;
            }
        }
    }

    struct Bzip2 {
        bz_stream s{};
        bool open = false;

        bool start(std::string& error)
        {
            s = bz_stream{};
            const int rc = BZ2_bzDecompressInit(&s, 0, 0);
            open = rc == BZ_OK;
            if (!open)
                error = std::format("bz2 didn't start: {}", rc);
            return open;
        }
        bool restart(std::string& error)
        {
            BZ2_bzDecompressEnd(&s);
            return start(error);
        }
        // 1 at the end of a stream, 0 to go on, -1 on an error.
        int step(const char* in, size_t inSize, char* outBuffer, size_t outSize, size_t& consumed, size_t& produced,
            std::string& error)
        {
            s.next_in = const_cast<char*>(in);
            s.avail_in = static_cast<unsigned int>(inSize);
            s.next_out = outBuffer;
            s.avail_out = static_cast<unsigned int>(outSize);
            const int rc = BZ2_bzDecompress(&s);
            consumed = inSize - s.avail_in;
            produced = outSize - s.avail_out;
            if (rc == BZ_STREAM_END)
                return 1;
            if (rc != BZ_OK) {
                error = std::format("bz2 doesn't decompress: {}", rc);
                return -1;
            }
            return 0;
        }
        ~Bzip2()
        {
            if (open)
                BZ2_bzDecompressEnd(&s);
        }
    };

    struct Gzip {
        z_stream s{};
        bool open = false;

        bool start(std::string& error)
        {
            s = z_stream{};
            const int rc = inflateInit2(&s, 16 + MAX_WBITS);  // 16: the gzip wrapper
            open = rc == Z_OK;
            if (!open)
                error = std::format("gzip didn't start: {}", rc);
            return open;
        }
        bool restart(std::string& error)
        {
            if (inflateReset(&s) == Z_OK)
                return true;
            error = "gzip didn't restart";
            return false;
        }
        int step(const char* in, size_t inSize, char* outBuffer, size_t outSize, size_t& consumed, size_t& produced,
            std::string& error)
        {
            s.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in));
            s.avail_in = static_cast<uInt>(inSize);
            s.next_out = reinterpret_cast<Bytef*>(outBuffer);
            s.avail_out = static_cast<uInt>(outSize);
            const int rc = inflate(&s, Z_NO_FLUSH);
            consumed = inSize - s.avail_in;
            produced = outSize - s.avail_out;
            if (rc == Z_STREAM_END)
                return 1;
            if (rc != Z_OK && rc != Z_BUF_ERROR) {  // a buffer error is no progress, run() tells an early end
                error = std::format("gzip doesn't decompress: {}{}", rc, s.msg ? std::format(", {}", s.msg) : "");
                return -1;
            }
            return 0;
        }
        ~Gzip()
        {
            if (open)
                inflateEnd(&s);
        }
    };
} // namespace

Compression detect(std::string_view bytes)
{
    if (bytes.size() >= 4 && bytes.starts_with("BZh") && bytes[3] >= '1' && bytes[3] <= '9')
        return Compression::Bzip2;
    if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0x1f && static_cast<unsigned char>(bytes[1]) == 0x8b)
        return Compression::Gzip;
    return Compression::None;
}

bool decompress(std::string_view bytes, Compression compression, std::string& out, std::string& error)
{
    switch (compression) {
    case Compression::Bzip2: {
        Bzip2 stream;
        return stream.start(error) && run(bytes, out, error, stream);
    }
    case Compression::Gzip: {
        Gzip stream;
        return stream.start(error) && run(bytes, out, error, stream);
    }
    case Compression::None: break;
    }
    out.assign(bytes);
    return true;
}

} // namespace indexer

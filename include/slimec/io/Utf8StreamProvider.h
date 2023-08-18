#pragma once

#include "StreamProvider.h"
#include "CodeConvert.h"

#include <stddef.h>
#include <algorithm>

namespace slimec {

template <typename InStreamPtr>
class Utf8StreamProvider final
    : public StreamProvider<Utf8StreamProvider<InStreamPtr>, char32_t> {
public:
    using base_type = StreamProvider<Utf8StreamProvider<InStreamPtr>, char32_t>;
    using char_type = typename base_type::char_type;

    static_assert(std::is_same_v<char_type, char32_t>);

    friend base_type;

public:
    Utf8StreamProvider(InStreamPtr &stream)
        : base_type(stream) {}

    Utf8StreamProvider(InStreamPtr &&stream)
        : base_type(stream) {}

    Utf8StreamProvider(Utf8StreamProvider &&other)
        : base_type(other) {}

    Utf8StreamProvider(Utf8StreamProvider &other)            = delete;
    Utf8StreamProvider &operator=(Utf8StreamProvider &other) = delete;

public:
    static size_t encodeAfter(char_type code, char *dest, size_t n) {
        return unicodeToUtf8(code, dest, n);
    }

private:
    char_type get() {
        using self = base_type;
        if (!self::tryReadNextByte()) { self::raiseError(); }

        auto byte = self::currentByte();
        if (((byte >> 7) & 0b1) == 0) { return byte; }

        const auto n = (byte & 0b11100000) == 0b11000000 ? 1
                     : (byte & 0b11110000) == 0b11100000 ? 2
                     : (byte & 0b11111000) == 0b11110000 ? 3
                     : (byte & 0b11111100) == 0b11111000 ? 4
                     : (byte & 0b11111110) == 0b11111100 ? 5
                                                         : 0;
        if (n > 0) {
            char_type code = self::currentByte() & ((1 << (7 - n)) - 1);
            bool      ok   = true;
            for (int i = 0; i < n; ++i) {
                ok &= self::tryReadNextByte();
                ok &= ((self::currentByte() >> 6) & 0b11) == 0b10;
                if (!ok) { break; }
                code <<= 6;
                code |= self::currentByte() & 0x3f;
            }
            if (ok) { return code; }
        }

        self::raiseError();
    }
};

} // namespace slimec

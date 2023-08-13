#include "CodeConvert.h"

#include <stdlib.h>
#include <algorithm>

namespace slime {

size_t unicodeToUtf8(char32_t code, char *dest, size_t n) {
    const auto expected = code < 0x80       ? 1
                        : code < 0x800      ? 2
                        : code < 0x10000    ? 3
                        : code < 0x200000   ? 4
                        : code < 0x4000000  ? 5
                        : code < 0x80000000 ? 6
                                            : 0;
    if (expected == 0) { abort(); }
    if (dest == nullptr) { return expected; }

    n = std::min<size_t>(expected, n);
    if (n == 0) { return 0; }

    for (int i = n - 1; i > 0; --i) {
        dest[i] = 0x80 | (code & 0x3f);
        code    >>= 6;
    }
    dest[0] = expected > 1 ? (((1 << expected) - 1) << (8 - expected)) | code
                           : code & 0x7f;
    return n;
}

} // namespace slime

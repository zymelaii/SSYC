#include "Utf8OutputHandler.h"
#include "CodeConvert.h"

#include <stdarg.h>
#include <stdlib.h>

namespace slime {

Utf8OutputHandler::Utf8OutputHandler(FILE* handle)
    : handle_{handle} {
    if (!handle_) { abort(); }
}

void Utf8OutputHandler::put(char ch) {
    fputc(ch, handle_);
}

void Utf8OutputHandler::put(char32_t charcode) {
    char   buf[4]{};
    size_t n = unicodeToUtf8(charcode, buf, 4);
    for (int i = 0; i < n; ++i) { put(buf[i]); }
}

void Utf8OutputHandler::print(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
}

void Utf8OutputHandler::println(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    print(fmt, ap);
    va_end(ap);
    put('\n');
}

void Utf8OutputHandler::print(const char* fmt, va_list ap) {
    vfprintf(handle_, fmt, ap);
}

} // namespace slime

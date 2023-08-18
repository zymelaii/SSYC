#pragma once

#include <iostream>

namespace slimec {

class Utf8OutputHandler {
public:
    Utf8OutputHandler(FILE* handle);

public:
    void put(char ch);
    void put(char32_t charcode);

    void print(const char* fmt, ...);
    void println(const char* fmt, ...);

protected:
    void print(const char* fmt, va_list ap);

private:
    FILE* const handle_;
};

} // namespace slimec

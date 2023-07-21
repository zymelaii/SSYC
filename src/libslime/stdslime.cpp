#include "stdslime.h"

#include <iostream>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <chrono>
#include <map>
#include <array>
#include <atomic>

#ifdef __WIN32__
#include <direct.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

static constexpr size_t MAX_TIMEPOINT = 1024;

using timepoint_table_t = std::map<std::pair<char*, int>, int>;
using timer_records_t   = std::array<std::chrono::microseconds, MAX_TIMEPOINT>;

static std::atomic_size_t __SLIME_TIMEPOINT_INDEX;
static timepoint_table_t  __SLIME_TIMEPOINT_TABLE;
static timer_records_t    __SLIME_TIMER_RECORDS;

int getch() {
    return static_cast<int>(getchar());
}

int getint() {
    int v = 0;
    scanf("%d", &v);
    return v;
}

float getfloat() {
    float v = 0.f;
    scanf("%a", &v);
    return v;
}

int getarray(int a[]) {
    const int len = getint();
    for (int i = 0; i < len; ++i) { a[i] = getint(); }
    return len;
}

int getfarray(float a[]) {
    const int len = getint();
    for (int i = 0; i < len; ++i) { a[i] = getfloat(); }
    return len;
}

void putint(int v) {
    printf("%d", v);
}

void putch(int v) {
    putchar(static_cast<char>(v));
}

void putfloat(float v) {
    printf("%a", v);
}

void putarray(int n, int a[]) {
    printf("%d:", n);
    for (int i = 0; i < n; ++i) { printf(" %d", a[i]); }
    putchar('\n');
}

void putfarray(int n, float a[]) {
    printf("%d:", n);
    for (int i = 0; i < n; ++i) { printf(" %a", a[i]); }
    putchar('\n');
}

void putf(char a[], ...) {
    va_list args;
    va_start(args, a);
    vfprintf(stdout, a, args);
    va_end(args);
}

void __slime_starttime(const char* file, int lineno) {
    const auto index = __SLIME_TIMEPOINT_INDEX++;
    assert(index >= 0 && index < MAX_TIMEPOINT);
    const auto key               = std::pair{const_cast<char*>(file), lineno};
    __SLIME_TIMEPOINT_TABLE[key] = index;
    __SLIME_TIMER_RECORDS[index] =
        std::chrono::system_clock::now().time_since_epoch();
}

void __slime_stoptime(const char* file, int lineno) {
    const auto timepoint = std::chrono::system_clock::now().time_since_epoch();
    int        startline = lineno + 1;
    for (const auto& [key, _] : __SLIME_TIMEPOINT_TABLE) {
        const auto& [src, line] = key;
        if (strcmp(file, src) != 0 || line > lineno) { continue; }
        if (lineno - startline < 0 || line > startline) { startline = line; }
    }
    const auto key = std::pair{const_cast<char*>(file), startline};
    assert(__SLIME_TIMEPOINT_TABLE.count(key) == 1);
    const auto index  = __SLIME_TIMEPOINT_TABLE[key];
    auto&      record = __SLIME_TIMER_RECORDS[index];
    record            = timepoint - record;
}

__attribute((constructor)) void __slime_main_ctor() {
    std::atomic_init(&__SLIME_TIMEPOINT_INDEX, 0);
}

__attribute((destructor)) void __slime_main_dtor() {
    for (const auto& [key, index] : __SLIME_TIMEPOINT_TABLE) {
        const auto& [file, lineno] = key;
        const auto dur             = __SLIME_TIMER_RECORDS[index];
        fprintf(
            stderr, "[%d] %s:%d: %lldus\n", index, file, lineno, dur.count());
    }
}

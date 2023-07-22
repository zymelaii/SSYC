#include "stdslime.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#define MAX_TIMEPOINT 1024

typedef struct timepoint_item_s {
    char* source;
    int   line;
    int   index;
} timepoint_item_t;

typedef size_t           timer_records_t[MAX_TIMEPOINT];
typedef timepoint_item_t timepoint_table_t[MAX_TIMEPOINT];

static size_t            __SLIME_TIMEPOINT_INDEX;
static timepoint_table_t __SLIME_TIMEPOINT_TABLE;
static timer_records_t   __SLIME_TIMER_RECORDS;

static size_t __us_now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int getch() {
    return (int)getchar();
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
    putchar((char)v);
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
    const size_t index = __SLIME_TIMEPOINT_INDEX++;
    assert(index >= 0 && index < MAX_TIMEPOINT);
    timepoint_item_t* item       = &__SLIME_TIMEPOINT_TABLE[index];
    item->source                 = (char*)file;
    item->line                   = lineno;
    item->index                  = index;
    __SLIME_TIMER_RECORDS[index] = __us_now();
}

void __slime_stoptime(const char* file, int lineno) {
    const size_t timepoint = __us_now();
    int          startline = lineno + 1;
    int          index     = -1;
    for (int i = 0; i < __SLIME_TIMEPOINT_INDEX; ++i) {
        const timepoint_item_t* item = &__SLIME_TIMEPOINT_TABLE[i];
        if (strcmp(file, item->source) != 0 || item->line > lineno) {
            continue;
        }
        if (lineno - startline < 0 || item->line > startline) {
            startline = item->line;
            index     = i;
        }
    }
    assert(index != -1);
    assert(timepoint > __SLIME_TIMER_RECORDS[index]);
    __SLIME_TIMER_RECORDS[index] = timepoint - __SLIME_TIMER_RECORDS[index];
}

__attribute((constructor)) void __slime_main_ctor() {
    __SLIME_TIMEPOINT_INDEX = 0;
}

__attribute((destructor)) void __slime_main_dtor() {
    for (int i = 0; i < __SLIME_TIMEPOINT_INDEX; ++i) {
        const timepoint_item_t* item   = &__SLIME_TIMEPOINT_TABLE[i];
        const char*             file   = item->source;
        const size_t            lineno = item->line;
        const size_t            index  = item->index;
        assert(index == i);
        const size_t dur = __SLIME_TIMER_RECORDS[item->index];
        fprintf(stderr, "[%zu] %s:%zu: %zuus\n", index, file, lineno, dur);
    }
}

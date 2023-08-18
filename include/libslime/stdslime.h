#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int   getch();
int   getint();
float getfloat();

int getarray(int a[]);
int getfarray(float a[]);

void putint(int v);
void putch(int v);
void putfloat(float v);

void putarray(int n, int a[]);
void putfarray(int n, float a[]);

void putf(char a[], ...);

void __slime_starttime(const char* file, int lineno);
void __slime_stoptime(const char* file, int lineno);

#ifndef NDEBUG
__attribute((constructor)) void __slime_main_ctor();
__attribute((destructor)) void  __slime_main_dtor();

#define starttime() __slime_starttime(__FILE__, __LINE__)
#define stoptime()  __slime_stoptime(__FILE__, __LINE__)
#else
#define starttime()
#define stoptime()
#endif

#ifdef __cplusplus
}
#endif

#pragma once

#include "pch.h"           // IWYU pragma: export
#include "ast_decl.h"      // IWYU pragma: export
#include "utils_conv2py.h" // IWYU pragma: export

#include <string_view>

#ifndef NDEBUG

#ifndef SSYC_PRINT_TOKEN
#define SSYC_PRINT_TOKEN(prompt)                                     \
 do {                                                                \
  const auto token_view = std::string_view(yytext, yytext + yyleng); \
  LOG(INFO) << prompt << ": " << token_view;                         \
 } while (0)
#endif

#ifndef SSYC_PRINT_REDUCE
#define SSYC_PRINT_REDUCE(target, rule) \
 do { LOG(WARNING) << "reduce rule " << #target << " <- " << rule; } while (0)
#endif

#else

#ifdef SSYC_PRINT_TOKEN
#undef SSYC_PRINT_TOKEN
#endif
#define SSYC_PRINT_TOKEN(...)

#ifdef SSYC_PRINT_REDUCE
#undef SSYC_PRINT_REDUCE
#endif
#define SSYC_PRINT_REDUCE(...)

#endif

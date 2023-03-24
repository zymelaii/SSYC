#pragma once

#include "ast_decl.h"

#include <ostream>

namespace ssyc::utils {

void conv2py(std::ostream &os, const ssyc::ast::Program *program);

} // namespace ssyc::utils

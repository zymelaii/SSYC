#pragma once

#include "ast_decl.h"

#include <ostream>

void conv2py(std::ostream &os, const ssyc::ast::Program *program);

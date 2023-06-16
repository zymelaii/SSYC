#pragma once

#include "../utils/list.h"
#include "decl.h"

namespace slime::ast {

using TopLevelDeclList = slime::utils::ListTrait<Decl*>;

struct TranslationUnit : public TopLevelDeclList {};

} // namespace slime::ast

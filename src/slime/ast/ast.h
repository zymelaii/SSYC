#pragma once

#include "../utils/list.h"
#include "../utils/traits.h"
#include "decl.h"

namespace slime::ast {

using TopLevelDeclList = slime::utils::ListTrait<Decl*>;

struct TranslationUnit
    : public TopLevelDeclList
    , public utils::BuildTrait<TranslationUnit> {};

} // namespace slime::ast

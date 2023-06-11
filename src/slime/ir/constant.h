#pragma once

#include "value.h"

namespace slime::ir {

class Constant : public Value {};

class ConstantData : public Constant {};

class ConstantFP final : public ConstantData {};

class ConstantInt final : public ConstantData {};

class ConstantArray final : public ConstantData {};

class GlobalValue : public Constant {};

class GlobalObject : public GlobalValue {};

class GlobalVariable final : public GlobalObject {};

class Function final : public GlobalObject {};

} // namespace slime::ir

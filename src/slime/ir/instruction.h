#pragma once

#include "user.h"

namespace slime::ir {

class Instruction : public User {};

class PhiInst final : public Instruction {};

class UnaryOpInst : public Instruction {};

class BinaryOpInst final : public Instruction {};

class AllocaInst final : public Instruction {};

class LoadInst final : public Instruction {};

class StoreInst final : public Instruction {};

class ReturnInst final : public Instruction {};

class SelectInst final : public Instruction {};

class BranchInst final : public Instruction {};

} // namespace slime::ir

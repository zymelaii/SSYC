#include "instruction.h"

namespace slime::ir {

Instruction *Value::asInstruction() {
    return reinterpret_cast<Instruction *>(patch());
}

GlobalObject *Value::asGlobalObject() {
    return static_cast<GlobalObject *>(this);
}

ConstantData *Value::asConstantData() {
    return static_cast<ConstantData *>(this);
}

GlobalVariable *Value::asGlobalVariable() {
    return static_cast<GlobalVariable *>(this);
}

Function *Value::asFunction() {
    return static_cast<Function *>(this);
}

Instruction *Value::tryIntoInstruction() {
    return isInstruction() ? static_cast<Instruction *>(patch()) : nullptr;
}

GlobalObject *Value::tryIntoGlobalObject() {
    return isGlobal() ? static_cast<GlobalObject *>(this) : nullptr;
}

ConstantData *Value::tryIntoConstantData() {
    return isReadOnly() && !isFunction() ? static_cast<ConstantData *>(this)
                                         : nullptr;
}

GlobalVariable *Value::tryIntoGlobalVariable() {
    return isGlobal() && !isFunction() ? static_cast<GlobalVariable *>(this)
                                       : nullptr;
}

Function *Value::tryIntoFunction() {
    return isFunction() ? static_cast<Function *>(this) : nullptr;
}

} // namespace slime::ir

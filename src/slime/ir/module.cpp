#include "module.h"
#include "user.h"

#include <string.h>

namespace slime::ir {

Module::Module(const char* name)
    : moduleName_{strdup(name)} {}

Module::~Module() {
    free(const_cast<char*>(moduleName_));
    moduleName_ = nullptr;

    for (auto& [k, v] : i32DataMap_) { delete v; }
    for (auto& [k, v] : f32DataMap_) { delete v; }
}

ConstantInt* Module::createInt(int32_t value) {
    if (i32DataMap_.count(value) == 0) {
        i32DataMap_[value] = ConstantData::createI32(value);
    }
    return i32DataMap_.at(value);
}

ConstantFloat* Module::createFloat(float value) {
    if (f32DataMap_.count(value) == 0) {
        f32DataMap_[value] = ConstantData::createF32(value);
    }
    return f32DataMap_.at(value);
}

bool Module::acceptFunction(Function* fn) {
    assert(fn != nullptr);
    assert(!fn->name().empty());
    if (functions_.count(fn->name()) != 0) {
        auto& prev = functions_.at(fn->name());
        //! try to replace declaration with definition
        if (prev->type()->equals(fn->proto()) && prev->size() == 0
            && fn->size() != 0) {
            prev = fn;
            return true;
        }
        return false;
    }
    functions_[fn->name()] = fn;
    return true;
}

bool Module::acceptGlobalVariable(GlobalVariable* var) {
    assert(var != nullptr);
    assert(!var->name().empty());
    if (globalVariables_.count(var->name()) != 0) { return false; }
    globalVariables_[var->name()] = var;
    return true;
}

Function* Module::lookupFunction(std::string_view name) {
    return functions_.count(name) > 0 ? functions_.at(name) : nullptr;
}

GlobalVariable* Module::lookupGlobalVariable(std::string_view name) {
    return globalVariables_.count(name) > 0 ? globalVariables_.at(name)
                                            : nullptr;
}

} // namespace slime::ir

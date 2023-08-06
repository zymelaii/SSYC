#include "49.h"
#include "51.h"
#include "47.h"

#include <string.h>

namespace slime::ir {

Module::Module(const char* name)
    : moduleName_{strdup(name)} {
    initializeBuiltinFunctions();
}

Module::~Module() {
    free(const_cast<char*>(moduleName_));
    moduleName_ = nullptr;

    for (auto& [k, v] : i32DataMap_) { delete v; }
    for (auto& [k, v] : f32DataMap_) { delete v; }
}

ConstantInt* Module::createI8(int8_t value) {
    if (i8DataMap_.count(value) == 0) {
        i8DataMap_[value] = ConstantData::createI32(value);
    }
    return i8DataMap_.at(value);
}

ConstantInt* Module::createI32(int32_t value) {
    if (i32DataMap_.count(value) == 0) {
        i32DataMap_[value] = ConstantData::createI32(value);
    }
    return i32DataMap_.at(value);
}

ConstantFloat* Module::createF32(float value) {
    if (f32DataMap_.count(value) == 0) {
        f32DataMap_[value] = ConstantData::createF32(value);
    }
    return f32DataMap_.at(value);
}

GlobalVariable* Module::createString(std::string_view value) {
    if (strDataMap_.count(value.data()) == 0) {
        auto type = ArrayType::create(
            IntegerType::get(IntegerKind::i8), value.size() + 1);
        auto name =
            strdup((".str." + std::to_string(strDataMap_.size())).c_str());
        auto str  = GlobalVariable::create(name, type, true);
        auto data = ConstantArray::create(type);
        for (int i = 0; i < value.size() + 1; ++i) {
            (*data)[i] = createI8(value[i]);
        }
        str->setInitData(data);
        auto ok = acceptGlobalVariable(str);
        assert(ok);
        strDataMap_[value.data()] = str;
    }
    return strDataMap_.at(value.data());
}

bool Module::acceptFunction(Function* fn) {
    assert(fn != nullptr);
    assert(!fn->name().empty());
    if (functions_.count(fn->name()) != 0) {
        auto& prev = functions_.at(fn->name());
        //! try to replace declaration with definition
        if (prev->type()->equals(fn->proto()) && prev->size() == 0
            && fn->size() != 0) {
            auto it = node_begin();
            while (it != node_end()) {
                if (it->value() == prev) {
                    it->value() = fn;
                    break;
                }
                ++it;
            }
            prev = fn;
            return true;
        }
        return false;
    }
    functions_[fn->name()] = fn;
    insertToTail(fn);
    return true;
}

bool Module::acceptGlobalVariable(GlobalVariable* var) {
    assert(var != nullptr);
    assert(!var->name().empty());
    if (globalVariables_.count(var->name()) != 0) { return false; }
    globalVariables_[var->name()] = var;
    insertToTail(var);
    return true;
}

Function* Module::lookupFunction(std::string_view name) {
    return functions_.count(name) > 0 ? functions_.at(name) : nullptr;
}

GlobalVariable* Module::lookupGlobalVariable(std::string_view name) {
    return globalVariables_.count(name) > 0 ? globalVariables_.at(name)
                                            : nullptr;
}

CallInst* Module::createMemset(Value* address, uint8_t value, size_t n) {
    assert(address != nullptr);
    auto fn = lookupFunction("memset");
    assert(fn != nullptr);
    auto call        = Instruction::createCall(fn);
    call->paramAt(0) = address;
    call->paramAt(1) = createI32(value);
    call->paramAt(2) = createI32(n);
    return call;
}

void Module::initializeBuiltinFunctions() {
    bool ok = false;
    //! void* memset(void*, int, size_t)
    auto builtinMemset = Function::create(
        "memset",
        FunctionType::create(
            Type::createPointerType(Type::getVoidType()),
            Type::createPointerType(Type::getVoidType()),
            Type::getIntegerType(),
            Type::getIntegerType()));
    ok = acceptFunction(builtinMemset);
    assert(ok);
}

} // namespace slime::ir
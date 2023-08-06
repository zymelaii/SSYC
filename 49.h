#pragma once

#include "88.h"
#include <stdint.h>
#include <string_view>
#include <map>

namespace slime::ir {

class Type;
class SequentialType;
class ArrayType;
class PointerType;
class FunctionType;

class Value;
class Use;
template <int N>
class User;

class BasicBlock;
class Parameter;

class Constant;
class ConstantData;
class ConstantInt;
class ConstantFloat;
class ConstantArray;
class GlobalObject;
class GlobalVariable;
class Function;

class Instruction;
class AllocaInst;
class LoadInst;
class StoreInst;
class RetInst;
class BrInst;
class GetElementPtrInst;
class AddInst;
class SubInst;
class MulInst;
class UDivInst;
class SDivInst;
class URemInst;
class SRemInst;
class FNegInst;
class FAddInst;
class FSubInst;
class FMulInst;
class FDivInst;
class FRemInst;
class ShlInst;
class LShrInst;
class AShrInst;
class AndInst;
class OrInst;
class XorInst;
class FPToUIInst;
class FPToSIInst;
class UIToFPInst;
class SIToFPInst;
class ICmpInst;
class FCmpInst;
class PhiInst;
class CallInst;

using GlobalObjectList = utils::ListTrait<GlobalObject*>;

class Module : public GlobalObjectList {
public:
    Module(const char* name);
    ~Module();

    inline std::string_view name() const;

    [[nodiscard]] ConstantInt*    createI8(int8_t value);
    [[nodiscard]] ConstantInt*    createI32(int32_t value);
    [[nodiscard]] ConstantFloat*  createF32(float value);
    [[nodiscard]] GlobalVariable* createString(std::string_view value);

    bool acceptFunction(Function* fn);
    bool acceptGlobalVariable(GlobalVariable* var);

    Function*       lookupFunction(std::string_view name);
    GlobalVariable* lookupGlobalVariable(std::string_view name);

    inline GlobalObjectList&       globalObjects();
    inline const GlobalObjectList& globalObjects() const;

    [[nodiscard]] CallInst* createMemset(
        Value* address, uint8_t value, size_t n);

protected:
    void initializeBuiltinFunctions();

private:
    const char* moduleName_;

    std::map<int32_t, ConstantInt*>        i32DataMap_;
    std::map<int8_t, ConstantInt*>         i8DataMap_;
    std::map<float, ConstantFloat*>        f32DataMap_;
    std::map<const char*, GlobalVariable*> strDataMap_;

    std::map<std::string_view, Function*>       functions_;
    std::map<std::string_view, GlobalVariable*> globalVariables_;
};

inline std::string_view Module::name() const {
    return moduleName_;
}

inline GlobalObjectList& Module::globalObjects() {
    return *this;
}

inline const GlobalObjectList& Module::globalObjects() const {
    return *this;
}

} // namespace slime::ir

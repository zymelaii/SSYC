#pragma once

#include <stdint.h>
#include <string_view>
#include <map>

namespace slime::ir {

class ConstantInt;
class ConstantFloat;
class Function;
class GlobalVariable;

class Module {
public:
    Module(const char* name);
    ~Module();

    inline std::string_view name() const;

    ConstantInt*   createI32(int32_t value);
    ConstantFloat* createF32(float value);

    bool acceptFunction(Function* fn);
    bool acceptGlobalVariable(GlobalVariable* var);

    Function*       lookupFunction(std::string_view name);
    GlobalVariable* lookupGlobalVariable(std::string_view name);

private:
    const char* moduleName_;

    std::map<int32_t, ConstantInt*> i32DataMap_;
    std::map<float, ConstantFloat*> f32DataMap_;

    std::map<std::string_view, Function*>       functions_;
    std::map<std::string_view, GlobalVariable*> globalVariables_;
};

inline std::string_view Module::name() const {
    return moduleName_;
}

} // namespace slime::ir

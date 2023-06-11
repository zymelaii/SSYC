#pragma once

#include "user.h"

#include <string_view>

namespace slime::ir {

class Argument;

class Constant : public User {
public:
    Constant(const Constant&)            = delete;
    Constant& operator=(const Constant&) = delete;

    Constant(Type* type, Use* operands, size_t totalOps)
        : User(type, operands, totalOps) {}
};

class ConstantData : public Constant {
public:
    ConstantData(const ConstantData&)            = delete;
    ConstantData& operator=(const ConstantData&) = delete;

    ConstantData(Type* type)
        : Constant(type, nullptr, 0) {}
};

class ConstantFP final : public ConstantData {
public:
    ConstantFP(const ConstantFP&)            = delete;
    ConstantFP& operator=(const ConstantFP&) = delete;

    ConstantFP(float value)
        : ConstantData(new Type(TypeID::Float))
        , value_{value} {}

private:
    float value_;
};

class ConstantInt final : public ConstantData {
public:
    ConstantInt(const ConstantInt&)            = delete;
    ConstantInt& operator=(const ConstantInt&) = delete;

    ConstantInt(int32_t value)
        : ConstantData(new IntegerType)
        , value_{value} {}

private:
    int32_t value_;
};

class ConstantArray final : public ConstantData {
public:
    ConstantArray(const ConstantArray&)            = delete;
    ConstantArray& operator=(const ConstantArray&) = delete;

    ConstantArray(ArrayType* type, std::vector<Constant*>& values)
        : ConstantData(type)
        , values_(std::move(values)) {}

private:
    std::vector<Constant*> values_;
};

class GlobalValue : public Constant {
public:
    GlobalValue(const GlobalValue&)            = delete;
    GlobalValue& operator=(const GlobalValue&) = delete;

    GlobalValue(
        Type* type, Use* operands, size_t totalOps, std::string_view name = "")
        : Constant(new PointerType(type), operands, totalOps)
        , valueType_{type}
        , name_{name} {}

private:
    Type*            valueType_;
    std::string_view name_;
};

class GlobalObject : public GlobalValue {
public:
    GlobalObject(const GlobalObject&)            = delete;
    GlobalObject& operator=(const GlobalObject&) = delete;

    GlobalObject(
        Type* type, Use* operands, size_t totalOps, std::string_view name = "")
        : GlobalValue(type, operands, totalOps, name) {}
};

class GlobalVariable final : public GlobalObject {
public:
    GlobalVariable(const GlobalVariable&)            = delete;
    GlobalVariable& operator=(const GlobalVariable&) = delete;

    GlobalVariable(
        Type*            type,
        bool             isConstant,
        Constant*        initVal = nullptr,
        std::string_view name    = "")
        : GlobalObject(type, new Use[1], initVal != nullptr, name)
        , isConstant_{isConstant} {
        //! TODO: check validity
        if (initVal != nullptr) { operandAt(0) = initVal; }
    }

private:
    bool isConstant_;
};

class Function final : public GlobalObject {
public:
    Function(const Function&)            = delete;
    Function& operator=(const Function&) = delete;

    Function(FunctionType* proto, std::string_view name = "")
        : GlobalObject(proto, nullptr, 0, name) {}

private:
    Argument* params_;
    size_t    totalParams_;
    //! FIXME: add symbol table and function body
};

} // namespace slime::ir

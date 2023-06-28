#include "minimal.h"

#include <assert.h>

namespace slime::ir {

namespace detail {
static constexpr int Alignment = 4;
} // namespace detail

Type::Type(TypeID id)
    : id{id}
    , containedTypes{nullptr}
    , totalContainedTypes{0} {}

Type *Type::getVoidType() {
    return new Type(TypeID::Void);
}

Type *Type::getLabelType() {
    return new Type(TypeID::Label);
}

Type *Type::getIntegerType() {
    return new Type(TypeID::Integer);
}

Type *Type::getFloatType() {
    return new Type(TypeID::Float);
}

FunctionType *
    Type::getFunctionType(Type *returnType, std::vector<Type *> &paramTypes) {
    return new FunctionType(returnType, paramTypes);
}

ArrayType *Type::getArrayType(Type *elementType, size_t length) {
    return new ArrayType(elementType, length);
}

PointerType *Type::getPointerType(Type *elementType) {
    return new PointerType(elementType);
}

bool Type::isPrimitiveType() const {
    return id != TypeID::Array && id != TypeID::Function
        && id != TypeID::Pointer;
}

bool Type::isElementType() const {
    return id != TypeID::Token && id != TypeID::Void && id != TypeID::Label;
}

FunctionType::FunctionType(Type *returnType, std::vector<Type *> &paramTypes)
    : Type(TypeID::Function)
    , returnType{returnType} {
    auto tmp            = std::move(paramTypes);
    totalContainedTypes = tmp.size();
    if (totalContainedTypes > 0) {
        containedTypes = new Type *[totalContainedTypes];
        for (int i = 0; i < totalContainedTypes; ++i) {
            containedTypes[i] = tmp[i];
        }
    }
}

Type *FunctionType::paramTypeAt(int index) {
    return containedTypes[index];
}

size_t FunctionType::totalParams() const {
    return totalContainedTypes;
}

SequentialType::SequentialType(TypeID id, Type *elementType)
    : Type(id) {
    totalContainedTypes = 1;
    containedTypes      = new Type *[totalContainedTypes];
    containedTypes[0]   = elementType;
}

Type *SequentialType::elementType() {
    return containedTypes[0];
}

ArrayType::ArrayType(Type *elementType, size_t length)
    : SequentialType(TypeID::Array, elementType)
    , length{length} {}

PointerType::PointerType(Type *elementType)
    : SequentialType(TypeID::Pointer, elementType) {}

Use::Use()
    : value{nullptr} {}

void Use::reset(Value *value) {
    if (this->value != nullptr) { this->value->removeUse(this); }
    this->value = value;
    if (this->value != nullptr) { this->value->addUse(this); }
}

Use &Use::operator=(Value *value) {
    reset(value);
    return *this;
}

Value::Value(Type *type)
    : type{type} {}

void Value::addUse(Use *use) {
    assert(use != nullptr && "add Use of nullptr");
    useList.push_back(use);
}

void Value::removeUse(Use *use) {
    useList.remove(use);
}

User::User(Type *type, size_t totalUse, Use *useList)
    : Value(type)
    , useList{useList}
    , totalUse{totalUse} {
    if (useList == nullptr && totalUse > 0) { useList = new Use[totalUse]; }
}

User::~User() {
    //! FIXME: 修正销毁方式
    if (useList != nullptr) {
        delete[] useList;
        useList = nullptr;
    }
}

Use &User::useAt(int index) {
    return useList[index];
}

BasicBlock::BasicBlock(Function *parent, std::string_view name)
    : Value(Type::getLabelType())
    , parent{parent} {
    this->name = name;
}

Parameter::Parameter(Type *type, Function *parent, std::string_view name)
    : Value(type)
    , parent{parent}
    , index{0} {
    this->name = name;
}

Constant::Constant(Type *type, size_t totalUse, Use *useList)
    : User(type, totalUse, useList) {}

ConstantData::ConstantData(Type *type)
    : Constant(type, 0, nullptr) {}

ConstantInt::ConstantInt(int32_t value)
    : ConstantData(Type::getIntegerType())
    , value{value} {}

ConstantFloat::ConstantFloat(float value)
    : ConstantData(Type::getFloatType())
    , value{value} {}

ConstantArray::ConstantArray(ArrayType *type, std::vector<Constant *> &elements)
    : Constant(type, static_cast<ArrayType *>(type)->length) {
    auto   arrayType = static_cast<ArrayType *>(type);
    auto   tmp       = std::move(elements);
    size_t n         = std::min(arrayType->length, tmp.size());
    for (int i = 0; i < n; ++i) {
        useAt(i) = tmp[i];
        //! FIXME: 处理未被初始化的值
    }
}

GlobalObject::GlobalObject(
    Type *type, size_t totalUse, Use *useList, std::string_view name)
    : Constant(type, totalUse, useList) {
    this->name = name;
}

GlobalVariable::GlobalVariable(
    Type *type, std::string_view name, bool isConstant, Constant *initValue)
    : GlobalObject(type, 1, nullptr, name)
    , isConstant{isConstant} {
    if (initValue != nullptr) {
        assert(initValue->type->id == type->id);
        useAt(0) = initValue;
    }
}

Function::Function(FunctionType *type, std::string_view name)
    : GlobalObject(type, 0, nullptr, name) {}

Instruction::Instruction(
    Type *type, InstructionID instType, size_t totalUse, Use *useList)
    : User(type, totalUse, useList)
    , instType{instType}
    , parent{nullptr} {}

std::string_view CmpInst::getPredicateName(ComparePredicationType predicate) {
    switch (predicate) {
        case ComparePredicationType::EQ: {
            return "eq";
        } break;
        case ComparePredicationType::NE: {
            return "ne";
        } break;
        case ComparePredicationType::UGT: {
            return "ugt";
        } break;
        case ComparePredicationType::UGE: {
            return "uge";
        } break;
        case ComparePredicationType::ULT: {
            return "ult";
        } break;
        case ComparePredicationType::ULE: {
            return "ule";
        } break;
        case ComparePredicationType::SGT: {
            return "sgt";
        } break;
        case ComparePredicationType::SGE: {
            return "sge";
        } break;
        case ComparePredicationType::SLT: {
            return "slt";
        } break;
        case ComparePredicationType::SLE: {
            return "sle";
        } break;
        case ComparePredicationType::FALSE: {
            return "false";
        } break;
        case ComparePredicationType::OEQ: {
            return "oeq";
        } break;
        case ComparePredicationType::OGT: {
            return "ogt";
        } break;
        case ComparePredicationType::OGE: {
            return "oge";
        } break;
        case ComparePredicationType::OLT: {
            return "olt";
        } break;
        case ComparePredicationType::OLE: {
            return "ole";
        } break;
        case ComparePredicationType::ONE: {
            return "one";
        } break;
        case ComparePredicationType::ORD: {
            return "ord";
        } break;
        case ComparePredicationType::UEQ: {
            return "ueq";
        } break;
        case ComparePredicationType::UNE: {
            return "une";
        } break;
        case ComparePredicationType::UNO: {
            return "uno";
        } break;
        case ComparePredicationType::TRUE: {
            return "true";
        } break;
    }
}

} // namespace slime::ir

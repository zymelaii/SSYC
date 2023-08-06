#pragma once

#include "53.h"

#include "88.h"
#include "89.h"
#include <stdint.h>
#include <stddef.h>
#include <memory>

namespace slime::ir {

class BasicBlock;
class Function;
class ConstantData;
class ConstantInt;
class ConstantFloat;
class ConstantArray;

using BasicBlockList   = utils::ListTrait<BasicBlock*>;
using ConstantDataList = utils::ListTrait<ConstantData*>;

class Constant : public User<0> {
public:
    Constant(Type* type, uint32_t tag)
        : User(type, tag) {}
};

class ConstantData : public Constant {
public:
    ConstantData(Type* type, uint32_t tag)
        : Constant(type, tag) {}

    static inline ConstantInt*   getBoolean(bool value);
    static inline ConstantInt*   createI8(int8_t data);
    static inline ConstantInt*   createI32(int32_t data);
    static inline ConstantFloat* createF32(float data);
};

class ConstantInt final
    : public ConstantData
    , public utils::BuildTrait<ConstantInt> {
public:
    ConstantInt(int32_t value, Type* type = Type::getIntegerType())
        : ConstantData(type, ValueTag::Immediate | 0)
        , value{value} {}

public:
    int32_t value;
};

class ConstantFloat final
    : public ConstantData
    , public utils::BuildTrait<ConstantFloat> {
public:
    ConstantFloat(float value)
        : ConstantData(Type::getFloatType(), ValueTag::Immediate | 0)
        , value{value} {}

public:
    float value;
};

class ConstantArray final
    : public ConstantData
    , public utils::BuildTrait<ConstantArray> {
public:
    ConstantArray(ArrayType* type)
        : ConstantData(type, ValueTag::ReadOnly | 0) {}

    ConstantData*& operator[](size_t index) {
        if (index >= values_.size()) { values_.resize(index + 1, nullptr); }
        return values_[index];
    }

    ConstantData* at(size_t index) {
        return index < values_.size() ? values_[index] : nullptr;
    }

    size_t size() const {
        return values_.size();
    }

private:
    std::vector<ConstantData*> values_;
};

class GlobalObject : public Constant {
public:
    GlobalObject(Type* type, uint32_t tag)
        : Constant(type, tag) {}
};

class GlobalVariable final
    : public GlobalObject
    , public utils::BuildTrait<GlobalVariable> {
public:
    GlobalVariable(std::string_view name, Type* type, bool constant = false)
        : GlobalObject(
            Type::createPointerType(type),
            ValueTag::Global | (constant ? ValueTag::ReadOnly | 0 : 0)) {
        setName(name);
    }

    void setInitData(ConstantData* data) {
        assert(data->type()->equals(type()->tryGetElementType()));
        data_ = data;
    }

    void setInitData(int32_t data) {
        setInitData(ConstantData::createI32(data));
    }

    void setInitData(float data) {
        setInitData(ConstantData::createF32(data));
    }

    const ConstantData* data() const {
        return data_;
    }

    bool isConst() const {
        return isConstant();
    }

private:
    ConstantData* data_;
};

//! NOTE: function may contains uses of global variable, but we do not care
//! about it at the definition stage
class Function final
    : public GlobalObject
    , public BasicBlockList
    , public utils::BuildTrait<Function> {
public:
    Function(std::string_view name, FunctionType* proto)
        : GlobalObject(proto, ValueTag::Function | 0)
        , params_(new Parameter[proto->totalParams()]) {
        setName(name);
    }

    inline BasicBlockList& basicBlocks() {
        return *this;
    }

    inline const BasicBlockList& basicBlocks() const {
        return *this;
    }

    inline BasicBlock* front() {
        assert(size() > 0);
        return head()->value();
    }

    inline BasicBlock* back() {
        assert(size() > 0);
        return tail()->value();
    }

    inline FunctionType* proto() const {
        return const_cast<Function*>(this)->type()->asFunctionType();
    }

    inline const Parameter* params() const {
        return &params_.get()[0];
    }

    inline size_t totalParams() const {
        return proto()->totalParams();
    }

    inline const Parameter* paramAt(size_t index) const {
        return index < proto()->totalParams() ? &params_.get()[index] : nullptr;
    }

    inline void setParam(std::string_view name, size_t index) {
        if (index >= totalParams()) { return; }
        auto& param = params_.get()[index];
        param.setName(name);
        param.resetValueTypeUnsafe(proto()->paramTypeAt(index));
        param.attachTo(this, index);
    }

private:
    std::unique_ptr<Parameter> params_;
};

inline ConstantInt* ConstantData::getBoolean(bool value) {
    static ConstantInt trueSingleton(true, Type::getBooleanType());
    static ConstantInt falseSingleton(false, Type::getBooleanType());
    return value ? &trueSingleton : &falseSingleton;
}

inline ConstantInt* ConstantData::createI8(int8_t data) {
    return ConstantInt::create(data, IntegerType::get(IntegerKind::i8));
}

inline ConstantInt* ConstantData::createI32(int32_t data) {
    return ConstantInt::create(data);
}

inline ConstantFloat* ConstantData::createF32(float data) {
    return ConstantFloat::create(data);
}

} // namespace slime::ir

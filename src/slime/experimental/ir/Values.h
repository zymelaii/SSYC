#pragma once

#include "Value.h"
#include "CFG.h"

#include <slime/experimental/utils/LinkedList.h>
#include <stddef.h>
#include <memory>
#include <vector>
#include <variant>

namespace slime::experimental::ir {

class Function;
class BasicBlock;
class Instruction;

using InstructionList = LinkedList<Instruction *>;
using BasicBlockList  = LinkedList<BasicBlock *>;

class BasicBlock final
    : public Value
    , public CFGNode {
public:
    inline BasicBlock(Function *parent);

    inline Function *parent() const;

    inline bool isInserted() const;

    inline InstructionList       &instructions();
    inline const InstructionList &instructions() const;

    void moveToHead();
    void moveToTail();

    bool insertOrMoveAfter(BasicBlock *block);
    bool insertOrMoveBefore(BasicBlock *block);

    bool remove();

private:
    Function *const            parent_;
    BasicBlockList::node_type *self_;
    InstructionList            instList_;
};

class Parameter final : public Value {
    friend class Function;

protected:
    inline Parameter();
    inline Parameter(Function *parent, size_t index);
    inline void attachTo(Function *parent, size_t index);

public:
    inline Function *parent() const;
    inline size_t    index() const;

private:
    Function *parent_;
    size_t    index_;
};

class Constant : public Value {
protected:
    inline Constant(Type *type, uint32_t tag);
};

class ConstantData : public Constant {
protected:
    inline ConstantData(Type *type, uint32_t tag);
};

class Immediate final : public ConstantData {
public:
    inline Immediate(Type *type);

    template <typename T>
    inline Immediate *with(datatype_of<T> data);

    template <typename T>
    void set(datatype_of<T> data);

    template <typename T>
    datatype_of<T> get() const;

private:
    std::variant<
        std::monostate,
        bool,
        int8_t,
        uint8_t,
        int16_t,
        uint16_t,
        int32_t,
        uint32_t,
        int64_t,
        uint64_t,
        float,
        double,
        long double>
        data_;
};

class ConstantArray final : public ConstantData {
public:
    inline ConstantArray(ArrayType *type);

    inline ConstantData      *&operator[](size_t index);
    inline const ConstantData *operator[](size_t index) const;

    inline ConstantData       *at(size_t index);
    inline const ConstantData *at(size_t index) const;

    inline size_t size() const;

private:
    std::vector<ConstantData *> dataArray_;
};

class GlobalObject : public Constant {
protected:
    inline GlobalObject(Type *type, uint32_t tag);
};

class GlobalVariable final : public GlobalObject {
public:
    inline GlobalVariable(
        std::string_view name, Type *type, bool readonly = false);

    inline GlobalVariable *with(ConstantData *data);

    inline const ConstantData *data() const;

private:
    ConstantData *data_;
};

class Function final : public GlobalObject {
public:
    inline Function(std::string_view name, FnType *proto);

    inline BasicBlock           *entry() const;
    inline BasicBlockList       &basicBlocks();
    inline const BasicBlockList &basicBlocks() const;

    inline const FnType *proto() const;

    inline size_t     totalParams() const;
    inline Parameter *paramAt(size_t index) const;

private:
    std::unique_ptr<Parameter[], std::default_delete<Parameter[]>> params_;

    BasicBlockList basicBlocks_;
};

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline BasicBlock::BasicBlock(Function *parent)
    : Value(Type::getVoidTy(), static_cast<uint32_t>(ValueTag::Label))
    , parent_{parent}
    , self_{nullptr}
    , instList_() {}

inline Function *BasicBlock::parent() const {
    return parent_;
}

inline bool BasicBlock::isInserted() const {
    return self_ != nullptr;
}

inline InstructionList &BasicBlock::instructions() {
    return instList_;
}

inline const InstructionList &BasicBlock::instructions() const {
    return instList_;
}

inline Parameter::Parameter()
    : Value(Type::getVoidTy(), static_cast<uint32_t>(ValueTag::Parameter))
    , parent_{nullptr}
    , index_{0} {}

inline Parameter::Parameter(Function *parent, size_t index)
    : Parameter() {
    attachTo(parent, index);
}

inline void Parameter::attachTo(Function *parent, size_t index) {
    parent_ = parent;
    index_  = index;
}

inline Function *Parameter::parent() const {
    return parent_;
}

inline size_t Parameter::index() const {
    return index_;
}

inline Constant::Constant(Type *type, uint32_t tag)
    : Value(type, tag) {}

inline ConstantData::ConstantData(Type *type, uint32_t tag)
    : Constant(type, tag | static_cast<uint32_t>(ValueTag::ReadOnly)) {}

inline Immediate::Immediate(Type *type)
    : ConstantData(type, static_cast<uint32_t>(ValueTag::Immediate)) {}

template <typename T>
inline Immediate *Immediate::with(datatype_of<T> data) {
    set<T>(data);
    return this;
}

template <typename T>
inline void Immediate::set(datatype_of<T> data) {
    if (auto ty = type()->tryInto<IntType>()) {
        switch ((ty->bitWidth() << 1) | ty->isSigned()) {
            case (0 << 1) | 0: {
                data_ = static_cast<datatype_of<UPtrType>>(data);
            } break;
            case (1 << 1) | 0: {
                data_ = static_cast<datatype_of<BoolType>>(data);
            } break;
            case (8 << 1) | 0: {
                data_ = static_cast<datatype_of<U8Type>>(data);
            } break;
            case (8 << 1) | 1: {
                data_ = static_cast<datatype_of<I8Type>>(data);
            } break;
            case (16 << 1) | 0: {
                data_ = static_cast<datatype_of<U16Type>>(data);
            } break;
            case (16 << 1) | 1: {
                data_ = static_cast<datatype_of<I16Type>>(data);
            } break;
            case (32 << 1) | 0: {
                data_ = static_cast<datatype_of<U32Type>>(data);
            } break;
            case (32 << 1) | 1: {
                data_ = static_cast<datatype_of<I32Type>>(data);
            } break;
            case (64 << 1) | 0: {
                data_ = static_cast<datatype_of<U64Type>>(data);
            } break;
            case (64 << 1) | 1: {
                data_ = static_cast<datatype_of<I64Type>>(data);
            } break;
            default: {
                unreachable();
            } break;
        }
    } else if (auto ty = type()->tryInto<FPType>()) {
        switch (ty->bitWidth()) {
            case 32: {
                data_ = static_cast<datatype_of<FP32Type>>(data);
            } break;
            case 64: {
                data_ = static_cast<datatype_of<FP64Type>>(data);
            } break;
            case 128: {
                data_ = static_cast<datatype_of<FP128Type>>(data);
            } break;
            default: {
                unreachable();
            } break;
        }
    } else {
        unreachable();
    }
}

template <typename T>
inline datatype_of<T> Immediate::get() const {
    if (auto ty = type()->tryInto<IntType>()) {
        switch ((ty->bitWidth() << 1) | ty->isSigned()) {
            case (0 << 1) | 0: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<UPtrType>>(data_));
            } break;
            case (1 << 1) | 0: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<BoolType>>(data_));
            } break;
            case (8 << 1) | 0: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<U8Type>>(data_));
            } break;
            case (8 << 1) | 1: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<I8Type>>(data_));
            } break;
            case (16 << 1) | 0: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<U16Type>>(data_));
            } break;
            case (16 << 1) | 1: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<I16Type>>(data_));
            } break;
            case (32 << 1) | 0: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<U32Type>>(data_));
            } break;
            case (32 << 1) | 1: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<I32Type>>(data_));
            } break;
            case (64 << 1) | 0: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<U64Type>>(data_));
            } break;
            case (64 << 1) | 1: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<I64Type>>(data_));
            } break;
            default: {
                unreachable();
            } break;
        }
    } else if (auto ty = type()->tryInto<FPType>()) {
        switch (ty->bitWidth()) {
            case 32: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<FP32Type>>(data_));
            } break;
            case 64: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<FP64Type>>(data_));
            } break;
            case 128: {
                return static_cast<datatype_of<T>>(
                    std::get<datatype_of<FP128Type>>(data_));
            } break;
            default: {
                unreachable();
            } break;
        }
    } else {
        unreachable();
    }
}

inline ConstantArray::ConstantArray(ArrayType *type)
    : ConstantData(type, 0)
    , dataArray_() {}

inline ConstantData *&ConstantArray::operator[](size_t index) {
    if (index >= dataArray_.size()) { dataArray_.resize(index + 1, nullptr); }
    return dataArray_[index];
}

inline const ConstantData *ConstantArray::operator[](size_t index) const {
    return const_cast<ConstantArray *>(this)->operator[](index);
}

inline ConstantData *ConstantArray::at(size_t index) {
    return index < dataArray_.size() ? dataArray_[index] : nullptr;
}

inline const ConstantData *ConstantArray::at(size_t index) const {
    return const_cast<ConstantArray *>(this)->at(index);
}

inline size_t ConstantArray::size() const {
    return dataArray_.size();
}

inline GlobalObject::GlobalObject(Type *type, uint32_t tag)
    : Constant(type, tag | static_cast<uint32_t>(ValueTag::Global)) {}

inline GlobalVariable::GlobalVariable(
    std::string_view name, Type *type, bool readonly)
    : GlobalObject(
        Type::getPtrTy(type),
        readonly ? static_cast<uint32_t>(ValueTag::ReadOnly) : 0)
    , data_{nullptr} {
    setName(name);
}

inline GlobalVariable *GlobalVariable::with(ConstantData *data) {
    assert(data != nullptr);
    assert(data->type()->signature() == type()->signature());
    data_ = data;
    return this;
}

inline const ConstantData *GlobalVariable::data() const {
    return data_;
}

inline Function::Function(std::string_view name, FnType *proto)
    : GlobalObject(proto, static_cast<uint32_t>(ValueTag::Function))
    , params_(new Parameter[proto->totalParams()])
    , basicBlocks_() {
    setName(name);
    for (int i = 0; i < proto->totalParams(); ++i) {
        params_[i].attachTo(this, i);
    }
}

inline BasicBlock *Function::entry() const {
    assert(basicBlocks_.size() > 0);
    return basicBlocks_.head()->value();
}

inline BasicBlockList &Function::basicBlocks() {
    return basicBlocks_;
}

inline const BasicBlockList &Function::basicBlocks() const {
    return basicBlocks_;
}

inline const FnType *Function::proto() const {
    return type()->as<FnType>();
}

inline size_t Function::totalParams() const {
    return proto()->totalParams();
}

inline Parameter *Function::paramAt(size_t index) const {
    assert(index < proto()->totalParams());
    return std::addressof(params_[index]);
}

} // namespace slime::experimental::ir

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::BasicBlock>() const {
    return isLabel();
}

emit(bool)
    slime::experimental::ir::ValueBase::is<slime::experimental::ir::Parameter>()
        const {
    return isParameter();
}

emit(bool)
    slime::experimental::ir::ValueBase::is<slime::experimental::ir::Constant>()
        const {
    return isGlobal() || isReadOnly();
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::ConstantData>() const {
    return !isGlobal() && isReadOnly();
}

emit(bool)
    slime::experimental::ir::ValueBase::is<slime::experimental::ir::Immediate>()
        const {
    return isImmediate();
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::ConstantArray>() const {
    return !isGlobal() && isReadOnly() && !isImmediate();
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::GlobalObject>() const {
    return isGlobal();
}

emit(bool) slime::experimental::ir::ValueBase::is<
    slime::experimental::ir::GlobalVariable>() const {
    return isGlobal() && !isFunction();
}

emit(bool)
    slime::experimental::ir::ValueBase::is<slime::experimental::ir::Function>()
        const {
    return isFunction();
}

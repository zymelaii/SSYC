#pragma once

#include "type.h"
#include "cfg.h"

#include <slime/utils/list.h>
#include <stdint.h>
#include <string_view>
#include <array>
#include <vector>
#include <set>
#include <list>

namespace slime::ir {

class Value;
class Use;
template <int N>
class User;
class Instruction;
class ConstantData;
class GlobalObject;
class GlobalVariable;
class Parameter;
class Function;

using UseList         = utils::ListTrait<Use *>;
using InstructionList = utils::ListTrait<Instruction *>;
using BasicBlockList  = utils::ListTrait<BasicBlock *>;

enum class ValueTag : uint32_t {
    //! static value
    Static = 0x8000 << 1,
    //! global object, always a function or global variable
    Global = Static | (0x8000 << 2),
    //! constant semantically
    Constant = 0x8000 << 3,
    //! read-only completely, usually store in .rodata segment
    ReadOnly = Constant | Static | (0x8000 << 4),
    //! value is a label
    Label = 0x8000 << 5,
    //! value is an immediate
    Immediate = ReadOnly | (0x8000 << 6),
    //! value is a function
    Function = Global | ReadOnly | (0x8000 << 7),
    //! value is from an instruction
    Instruction = 0x8000 << 8,
    //! value is from a cmp instruction
    CompareInst = Instruction | (0x8000 << 9),
    //! value is a parameter
    Parameter = 0x8000 << 10,
};

inline uint32_t operator|(ValueTag tag1, ValueTag tag2);
inline uint32_t operator|(uint32_t tag1, ValueTag tag2);
inline uint32_t operator|(ValueTag tag1, uint32_t tag2);

class Value {
protected:
    inline Value(Type *type, uint32_t tag = 0);

public:
    inline const UseList &uses() const;

    inline void addUse(Use *use) const;
    inline void removeUse(Use *use) const;

    inline bool usedBy(const Use *use) const;

    template <int N>
    inline bool usedBy(const User<N> *user) const {
        return user->contains(this);
    }

    inline Type            *type();
    inline std::string_view name();
    inline uint32_t         tag() const;
    inline int32_t          id() const;
    inline uint32_t         version() const;

    inline bool isStatic() const;
    inline bool isGlobal() const;
    inline bool isConstant() const;
    inline bool isReadOnly() const;
    inline bool isLabel() const;
    inline bool isImmediate() const;
    inline bool isFunction() const;
    inline bool isInstruction() const;
    inline bool isCompareInst() const;
    inline bool isParameter() const;

    Instruction    *asInstruction();
    GlobalObject   *asGlobalObject();
    ConstantData   *asConstantData();
    GlobalVariable *asGlobalVariable();
    Function       *asFunction();

    Instruction    *tryIntoInstruction();
    GlobalObject   *tryIntoGlobalObject();
    ConstantData   *tryIntoConstantData();
    GlobalVariable *tryIntoGlobalVariable();
    Function       *tryIntoFunction();

    inline Value       *decay();
    inline const Value *decay() const;

    inline void  setPatch(void *patch) const;
    inline void *patch() const;

    inline void setName(std::string_view name, bool force = false);
    inline void setIdUnsafe(int32_t id, uint32_t version = 0);
    inline void resetValueTypeUnsafe(Type *valueType);

private:
    std::string_view name_      = "";
    int32_t          id_        = 0;
    uint32_t         version_   = 0;
    Type            *valueType_ = nullptr;
    uint32_t         tag_       = 0;
    mutable UseList  useList_;

    //! WARNING: under no circumstances should patch_ be initilaized since it
    //! might be modified by the external object at any time, initialization
    //! will probably cause the modification invalid
    //! e.g. an instruction user is orderly constructed by Instruction, User<N>,
    //! etc, where ctor of User<N> will construct the Value instance which may
    //! cover the modification of Instruction ctor
    mutable void *patch_;
};

class Use {
public:
    inline void   reset(const Value *value = nullptr);
    inline Use   &operator=(const Value *value);
    inline Use   &operator=(Use &use); //<! always move the use
    inline Value *value() const;
    inline        operator Value *() const;
    inline Value *operator->() const;

private:
    const Value *value_ = nullptr;
};

template <int N>
class User : public Value {
protected:
    User(Type *type, uint32_t tag)
        : Value(type, tag) {}

public:
    Use *op() const {
        return const_cast<Use *>(operands_.data());
    }

    template <int I>
    Use &op() const {
        static_assert(I >= 0 && I < N);
        return const_cast<Use &>(operands_[I]);
    }

    bool contains(const Value *use) const {
        for (int i = 0; i < N; ++i) {
            if (use == operands_[i]) { return true; }
        }
        return false;
    }

    constexpr size_t totalUse() const {
        return N;
    }

private:
    std::array<Use, N> operands_;
};

template <>
class User<-1> : public Value {
protected:
    User(Type *type, uint32_t tag)
        : Value(type, tag) {}

public:
    Use *op() const {
        return const_cast<Use *>(operands_.data());
    }

    bool contains(const Value *use) const {
        for (const auto &e : operands_) {
            if (use == e) { return true; }
        }
        return false;
    }

    size_t totalUse() const {
        return operands_.size();
    }

    void resize(size_t n) {
        for (int i = n - 1; i < operands_.size(); ++i) { operands_[i].reset(); }
        operands_.resize(n);
    }

private:
    std::vector<Use> operands_;
};

template <>
class User<0> : public Value {
protected:
    User(Type *type, uint32_t tag)
        : Value(type, tag) {}

public:
    Use *op() const {
        return nullptr;
    }

    constexpr bool contains(const Use *use) const {
        return false;
    }

    constexpr size_t totalUse() const {
        return 0;
    }
};

template <>
class User<1> : public Value {
protected:
    User(Type *type, uint32_t tag)
        : Value(type, tag) {}

public:
    Use *op() const {
        return const_cast<Use *>(&operand_);
    }

    Use &operand() const {
        return const_cast<Use &>(operand_);
    }

    bool contains(const Value *use) const {
        return use == operand();
    }

    constexpr size_t totalUse() const {
        return 1;
    }

private:
    Use operand_;
};

template <>
class User<2> : public Value {
protected:
    User(Type *type, uint32_t tag)
        : Value(type, tag) {}

public:
    Use *op() const {
        return const_cast<Use *>(operands_);
    }

    Use &lhs() const {
        return const_cast<Use &>(operands_[0]);
    }

    Use &rhs() const {
        return const_cast<Use &>(operands_[1]);
    }

    bool contains(const Value *use) const {
        return use == lhs() || use == rhs();
    }

    constexpr size_t totalUse() const {
        return 2;
    }

private:
    Use operands_[2];
};

template <>
class User<3> : public Value {
protected:
    User(Type *type, uint32_t tag)
        : Value(type, tag) {}

public:
    Use *op() const {
        return const_cast<Use *>(operands_);
    }

    template <int I>
    Use &op() const {
        static_assert(I >= 0 && I < 3);
        return const_cast<Use &>(operands_[I]);
    }

    bool contains(const Value *use) const {
        return use == op<0>() || use == op<1>() || use == op<2>();
    }

    constexpr size_t totalUse() const {
        return 3;
    }

private:
    Use operands_[3];
};

class BasicBlock final
    : public Value
    , public CFGNode
    , public InstructionList
    , public utils::BuildTrait<BasicBlock> {
public:
    inline BasicBlock(Function *parent);

    inline Function *parent() const;

    inline bool isInserted() const;

    inline InstructionList       &instructions();
    inline const InstructionList &instructions() const;

    void insertOrMoveToHead();
    void insertOrMoveToTail();

    bool insertOrMoveAfter(BasicBlock *block);
    bool insertOrMoveBefore(BasicBlock *block);

    bool remove();

private:
    Function *const            parent_;
    BasicBlockList::node_type *node_;
};

class Parameter final
    : public Value
    , public utils::BuildTrait<Parameter> {
public:
    inline Parameter();

    inline Function *parent() const;
    inline size_t    index() const;

    inline void attachTo(Function *fn, size_t index);

    inline void setName(std::string_view name);

private:
    Function *parent_;
    size_t    index_;
};

inline uint32_t operator|(ValueTag tag1, ValueTag tag2) {
    return static_cast<uint32_t>(tag1) | static_cast<uint32_t>(tag2);
}

inline uint32_t operator|(uint32_t tag1, ValueTag tag2) {
    return tag1 | static_cast<uint32_t>(tag2);
}

inline uint32_t operator|(ValueTag tag1, uint32_t tag2) {
    return static_cast<uint32_t>(tag1) | tag2;
}

inline Value::Value(Type *type, uint32_t tag)
    : valueType_{type}
    , tag_{tag} {}

inline const UseList &Value::uses() const {
    return useList_;
}

inline void Value::addUse(Use *use) const {
    for (auto e : useList_) {
        if (e == use) { return; }
    }
    useList_.insertToTail(use);
}

inline void Value::removeUse(Use *use) const {
    for (auto it = useList_.node_begin(); it != useList_.node_end(); ++it) {
        if (it->value() == use) {
            it->removeFromList();
            return;
        }
    }
}

inline bool Value::usedBy(const Use *use) const {
    for (auto e : useList_) {
        if (e == use) { return true; }
    }
    return false;
}

inline Type *Value::type() {
    return valueType_;
}

inline std::string_view Value::name() {
    return name_;
}

inline uint32_t Value::tag() const {
    return tag_;
}

inline int32_t Value::id() const {
    return id_;
}

inline uint32_t Value::version() const {
    return version_;
}

inline bool Value::isStatic() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Static);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isGlobal() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Global);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isConstant() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Constant);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isReadOnly() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::ReadOnly);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isLabel() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Label);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isImmediate() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Immediate);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isFunction() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Function);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isInstruction() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Instruction);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isCompareInst() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::CompareInst);
    return (tag_ & testFlag) == testFlag;
}

inline bool Value::isParameter() const {
    auto testFlag = static_cast<uint32_t>(ValueTag::Parameter);
    return (tag_ & testFlag) == testFlag;
}

inline Value *Value::decay() {
    return static_cast<Value *>(this);
}

inline const Value *Value::decay() const {
    return static_cast<const Value *>(this);
}

inline void Value::setPatch(void *patch) const {
    patch_ = patch;
}

inline void *Value::patch() const {
    return patch_;
}

inline void Value::setName(std::string_view name, bool force) {
    //! FIXME: handle multiple rename
    if (name_.empty() || force) { name_ = name; }
}

inline void Value::setIdUnsafe(int32_t id, uint32_t version) {
    id_      = id;
    version_ = version;
}

inline void Value::resetValueTypeUnsafe(Type *valueType) {
    valueType_ = valueType;
}

inline void Use::reset(const Value *value) {
    if (value != value_) {
        if (value_ != nullptr) { value_->removeUse(this); }
        if (value != nullptr) { value->addUse(this); }
        value_ = value;
    }
}

inline Use &Use::operator=(const Value *value) {
    reset(value);
    return *this;
}

inline Use &Use::operator=(Use &use) {
    if (this != std::addressof(use)) {
        use.reset();
        reset(use.value_);
    }
    return *this;
}

inline Value *Use::value() const {
    return const_cast<Value *>(value_);
}

inline Value *Use::operator->() const {
    return value();
}

inline Use::operator Value *() const {
    return value();
}

inline BasicBlock::BasicBlock(Function *parent)
    : Value(Type::getLabelType(), ValueTag::Label | 0)
    , parent_{parent}
    , node_{nullptr} {}

inline Function *BasicBlock::parent() const {
    return parent_;
}

inline bool BasicBlock::isInserted() const {
    return node_ != nullptr;
}

inline InstructionList &BasicBlock::instructions() {
    return *this;
}

inline const InstructionList &BasicBlock::instructions() const {
    return *this;
}

inline Parameter::Parameter()
    : Value(Type::getVoidType(), ValueTag::Parameter | 0) {}

inline Function *Parameter::parent() const {
    return parent_;
}

inline size_t Parameter::index() const {
    return index_;
}

inline void Parameter::attachTo(Function *fn, size_t index) {
    //! NOTE: this is a pseudo attach action, the realy one is done by Function
    parent_ = fn;
    index_  = index;
}

inline void Parameter::setName(std::string_view name) {
    Value::setName(name, true);
}

} // namespace slime::ir

#pragma once

#include "28.h"
#include "30.h"

#include "40.h"
#include "39.h"
#include <string_view>
#include <stdint.h>
#include <assert.h>

namespace slime::experimental::ir {

using UseList = LinkedList<Use *>;

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
    //! value is a user
    User = 0x8000 << 11,
};

class ValueBaseImpl {
protected:
    inline ValueBaseImpl(Type *type, uint32_t tag);

public:
    inline uint32_t id() const;
    inline Type    *type() const;

    inline std::string_view name() const;
    inline void             setName(std::string_view name);

    inline bool isStatic() const;
    inline bool isGlobal() const;
    inline bool isConstant() const;
    inline bool isReadOnly() const;
    inline bool isLabel() const;
    inline bool isImmediate() const;
    inline bool isFunction() const;
    inline bool isInstruction() const;
    inline bool isParameter() const;
    inline bool isUser() const;

    void addUse(Use *use) const;
    void removeUse(Use *use) const;

    inline UseList       &uses();
    inline const UseList &uses() const;

    inline UseList::iterator use_begin();
    inline UseList::iterator use_end();

    inline UseList::const_iterator use_begin() const;
    inline UseList::const_iterator use_end() const;

private:
    std::string_view name_;
    int32_t          id_;
    uint32_t         version_;
    Type            *type_;
    uint32_t         tag_;
    mutable UseList  useList_;
};

using ValueBase = UniversalTryIntoTraitWrapper<ValueBaseImpl>;

//! alias for forward declaration
class Value : public ValueBase {
protected:
    inline Value(Type *type, uint32_t tag);
};

} // namespace slime::experimental::ir

namespace slime::experimental::ir {

inline ValueBaseImpl::ValueBaseImpl(Type *type, uint32_t tag)
    : name_()
    , id_{0}
    , version_{0}
    , type_{type}
    , tag_{tag}
    , useList_() {}

inline uint32_t ValueBaseImpl::id() const {
    return id_;
}

inline std::string_view ValueBaseImpl::name() const {
    return name_;
}

inline Type *ValueBaseImpl::type() const {
    return type_;
}

inline void ValueBaseImpl::setName(std::string_view name) {
    name_ = name;
}

inline bool ValueBaseImpl::isStatic() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Static);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isGlobal() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Global);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isConstant() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Constant);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isReadOnly() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::ReadOnly);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isLabel() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Label);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isImmediate() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Immediate);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isFunction() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Function);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isInstruction() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Instruction);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isParameter() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::Parameter);
    return (tag_ & testFlag) == testFlag;
}

inline bool ValueBaseImpl::isUser() const {
    const auto testFlag = static_cast<uint32_t>(ValueTag::User);
    return (tag_ & testFlag) == testFlag;
}

inline UseList &ValueBaseImpl::uses() {
    return useList_;
}

inline UseList::iterator ValueBaseImpl::use_begin() {
    return useList_.begin();
}

inline UseList::iterator ValueBaseImpl::use_end() {
    return useList_.end();
}

inline const UseList &ValueBaseImpl::uses() const {
    return useList_;
}

inline UseList::const_iterator ValueBaseImpl::use_begin() const {
    return useList_.cbegin();
}

inline UseList::const_iterator ValueBaseImpl::use_end() const {
    return useList_.cend();
}

inline Value::Value(Type *type, uint32_t tag)
    : ValueBase(type, tag) {}

} // namespace slime::experimental::ir
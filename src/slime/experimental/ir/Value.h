#pragma once

#include "Type.h"
#include "Use.h"

#include <slime/experimental/utils/TryIntoTrait.h>
#include <slime/experimental/utils/LinkedList.h>
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
};

class ValueBaseImpl {
protected:
    ValueBaseImpl(Type *type, uint32_t tag)
        : type_{type}
        , tag_{tag} {}

public: //<! property getter
    auto id() const {
        return id_;
    }

    auto name() const {
        return name_;
    }

    auto type() const {
        return type_;
    }

public: //<! use
    void addUse(Use *use) const;
    void removeUse(Use *use) const;

    UseList &uses() {
        return useList_;
    }

    auto use_begin() {
        return useList_.begin();
    }

    auto use_end() {
        return useList_.end();
    }

    const UseList &uses() const {
        return useList_;
    }

    auto use_begin() const {
        return useList_.cbegin();
    }

    auto use_end() const {
        return useList_.cend();
    }

private:
    std::string_view name_    = "";
    int32_t          id_      = 0;
    uint32_t         version_ = 0;
    Type            *type_    = nullptr;
    uint32_t         tag_     = 0;

    mutable UseList useList_;
};

using ValueBase = UniversalTryIntoTraitWrapper<ValueBaseImpl>;

//! alias for forward declaration
class Value : public ValueBase {
protected:
    Value(Type *type, uint32_t tag)
        : ValueBase(type, tag) {}
};

} // namespace slime::experimental::ir

#pragma once

#include "../utils/list.h"
#include "type.h"
#include "instruction.def"

#include <stdint.h>
#include <assert.h>
#include <list>
#include <vector>
#include <string_view>

namespace slime::ir {

struct Value;
struct BasicBlock;
struct Instruction;
struct Function;
struct Instruction;
struct FunctionType;
struct ArrayType;
struct PointerType;

using InstructionList = slime::utils::ListTrait<Instruction *>;

inline namespace utils {

template <typename T>
inline uint32_t to_u32(const T &e) {
    return static_cast<uint32_t>(e);
}

template <typename T>
inline T from_u32(uint32_t e) {
    return static_cast<T>(e);
}

}; // namespace utils

//! use for SSA value
//! NOTE: 一对一关联使用关系
struct Use {
    Use();

    //! 重置使用关系
    void reset(Value *value = nullptr);

    Use &operator=(Value *value);

    Value *value; //<! 使用的 Value
};

//! SSA value
struct Value {
    Value(Type *type);

    //! 添加 Use
    void addUse(Use *use);

    //! 移除 Use
    void removeUse(Use *use);

    Type            *type;    //<! 值类型
    std::list<Use *> useList; //<! 关联的使用列表（被使用）
    std::string_view name;    //<! 值的命名，为空时由自动编号给出
                           //<! FIXME: 难以处理全局符号冲突等情况
};

//! user of SSA value
struct User : public Value {
    User(Type *type, size_t totalUse, Use *useList = nullptr);
    ~User();

    Use &useAt(int index);

    Use   *useList;  //!< 关联的使用列表（使用）
    size_t totalUse; //!< 关联的 Use 数
};

//! NOTE: BasicBlock 是由一个唯一的 label 引导的基本块
struct BasicBlock
    : public Value
    , public InstructionList {
    BasicBlock(Function *parent, std::string_view name = "");

    Function *parent; //<! 所属函数
};

//! parameter of function
//! NOTE: Parameter 是 Function 的形参，用于简化参数表示及转译处理
struct Parameter : public Value {
    Parameter(Type *type, Function *parent, std::string_view name = "");

    Function *parent; //<! 所属函数
    int       index;  //<! 参数位置（从 0 开始）
};

//! NOTE: 这里的 Constant 具有值为常量和地址为常量两种含义
struct Constant : public User {
    Constant(Type *type, size_t totalUse, Use *useList = nullptr);
};

//! constant data
struct ConstantData : public Constant {
    ConstantData(Type *type);
};

//! integer constant
struct ConstantInt : public ConstantData {
    ConstantInt(int32_t value);

    int32_t value;
};

//! 32-bit floating point constant
struct ConstantFloat : public ConstantData {
    ConstantFloat(float value);

    float value;
};

//! fixed-length array
struct ConstantArray : public Constant {
    ConstantArray(ArrayType *type, std::vector<Constant *> &elements);
};

//! NOTE: 全局值包括全局变量、常量和函数
struct GlobalObject : public Constant {
    GlobalObject(
        Type *type, size_t totalUse, Use *useList, std::string_view name);
};

struct GlobalVariable : public GlobalObject {
    GlobalVariable(
        Type            *type,
        std::string_view name,
        bool             isConstant,
        Constant        *initValue = nullptr);

    bool isConstant;
};

struct Function : public GlobalObject {
    Function(FunctionType *type, std::string_view name);

    std::vector<Parameter *> params; //<! 参数列表
    std::list<BasicBlock *>  blocks; //<! 基本块列表（顺序）
};

struct Instruction : public User {
    Instruction(
        Type         *type,
        InstructionID instType,
        size_t        totalUse,
        Use          *useList = nullptr);

    BasicBlock   *parent;   //<! 所属的基本块
    InstructionID instType; //<! 指令类型
};

struct AllocaInst : public Instruction {
    AllocaInst(Type *type)
        : Instruction(Type::createPointerType(type), InstructionID::Alloca, 0) {
    }
};

struct LoadInst : public Instruction {
    LoadInst(Type *type, Value *address)
        : Instruction(type, InstructionID::Load, 1) {
        useAt(0) = address;
    }
};

struct StoreInst : public Instruction {
    StoreInst(Value *value, Value *address)
        : Instruction(Type::getVoidType(), InstructionID::Store, 2) {
        useAt(0) = value;
        useAt(1) = address;
    }
};

struct ReturnInst : public Instruction {
    ReturnInst(Value *returnValue)
        : Instruction(
            Type::getVoidType(), InstructionID::Ret, returnValue != nullptr) {
        if (returnValue != nullptr) { useAt(0) = returnValue; }
    }
};

struct BranchInst : public Instruction {
    BranchInst(BasicBlock *dest)
        : Instruction(Type::getVoidType(), InstructionID::Br, 1) {
        useAt(0) = dest;
    }

    BranchInst(
        Value *condition, BasicBlock *branchOnTrue, BasicBlock *branchOnFalse)
        : Instruction(Type::getVoidType(), InstructionID::Br, 3) {
        useAt(0) = condition;
        useAt(1) = branchOnTrue;
        useAt(2) = branchOnFalse;
    }
};

struct GetElementPtrInst : public Instruction {
    GetElementPtrInst(Value *address, Value *index)
        : Instruction(
            Type::createPointerType(getElementType(address)),
            InstructionID::GetElementPtr,
            3) {
        useAt(0) = address;
        useAt(1) = new ConstantInt(0);
        useAt(2) = index;
    }

    static Type *getElementType(Value *value) {
        auto ptr  = static_cast<PointerType *>(value->type);
        auto type = static_cast<SequentialType *>(ptr->elementType());
        return type->elementType();
    }
};

struct BinaryOperatorInst : public Instruction {
    BinaryOperatorInst(InstructionID op, Value *lhs, Value *rhs)
        : Instruction(indicateResultType(lhs, rhs), op, 2) {
        useAt(0) = lhs;
        useAt(1) = rhs;
    }

    static Type *indicateResultType(Value *lhs, Value *rhs) {
        assert(
            lhs->type->isBuiltinType() && rhs->type->isBuiltinType()
            && "binary-operator instruction accepts only float and i32");
        if (lhs->type->isFloat() || rhs->type->isFloat()) {
            return Type::getFloatType();
        } else {
            return Type::getIntegerType();
        }
    }
};

struct CmpInst : public Instruction {
    CmpInst(
        InstructionID          id,
        ComparePredicationType predicate,
        Value                 *lhs,
        Value                 *rhs)
        : Instruction(Type::getIntegerType(), id, 2)
        , predicate{predicate} {
        useAt(0) = lhs;
        useAt(1) = rhs;
    }

    static std::string_view getPredicateName(ComparePredicationType predicate);

    ComparePredicationType predicate;
};

struct ICmpInst : public CmpInst {
    ICmpInst(ComparePredicationType predicate, Value *lhs, Value *rhs)
        : CmpInst(InstructionID::ICmp, predicate, lhs, rhs) {}
};

struct FCmpInst : public CmpInst {
    FCmpInst(ComparePredicationType predicate, Value *lhs, Value *rhs)
        : CmpInst(InstructionID::FCmp, predicate, lhs, rhs) {}
};

struct PhiInst : public Instruction {
    PhiInst(Value *firstValue, BasicBlock *block)
        : Instruction(firstValue->type, InstructionID::Phi, 2)
        , totalIncomings{1} {
        useAt(0) = firstValue;
        useAt(1) = block;
    }

    void addIncomingBlock(Value *value, BasicBlock *block) {
        if (totalIncomings * 2 == totalUse) {
            auto uses = new Use[totalUse + 2];
            std::copy(useList, useList + totalUse, uses);
            totalUse += 2;
            delete[] useList;
            useList = uses;
        }
        useAt(totalIncomings * 2)     = value;
        useAt(totalIncomings * 2 + 1) = block;
        ++totalIncomings;
    }

    void removeIncomingBlock(size_t index) {
        assert(index < totalIncomings);
        if (index + 1 == totalIncomings) {
            useAt(index * 2).reset();
            useAt(index * 2 + 1).reset();
        } else {
            auto &v              = useAt((totalIncomings - 1) * 2);
            auto &b              = useAt((totalIncomings - 1) * 2 + 1);
            useAt(index * 2)     = v.value;
            useAt(index * 2 + 1) = b.value;
            v.reset();
            b.reset();
        }
        --totalIncomings;
    }

    size_t totalIncomings;
};

struct CallInst : public Instruction {
    CallInst(Function *func, const std::vector<Value *> &args)
        : Instruction(
            static_cast<FunctionType *>(func->type)->returnType(),
            InstructionID::Call,
            func->params.size() + 1) {
        assert(totalUse - 1 == args.size());
        useAt(0) = func;
        for (int i = 0; i < args.size(); ++i) { useAt(i + 1) = args[i]; }
    }
};

} // namespace slime::ir

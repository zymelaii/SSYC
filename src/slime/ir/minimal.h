#pragma once

/*!
 * \brief IR 单文件版
 *
 * 懒得边设计边调格式边实现，先把结果跑出来，以后再重构
 */

#include <stdint.h>
#include <list>
#include <vector>
#include <string_view>

namespace slime::ir {

struct Value;
struct Function;
struct Instruction;
struct FunctionType;
struct ArrayType;
struct PointerType;

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

enum class TypeID {
    //! 基础类型
    Token,
    Label,
    Void,
    Integer, //<! 当前仅支持 i32
    Float,   //<! 当前仅支持 f32
    //! 复合类型
    Array,
    Function,
    Pointer, //<! NOTE: 暂未实现
};

struct Type {
    Type(TypeID id = TypeID::Token);

    static Type *getVoidType();
    static Type *getLabelType();
    static Type *getIntegerType();
    static Type *getFloatType();
    static FunctionType *
        getFunctionType(Type *returnType, std::vector<Type *> &paramTypes);
    static ArrayType   *getArrayType(Type *elementType, size_t length);
    static PointerType *getPointerType(Type *elementType);

    //! 是否是基础类型
    bool isPrimitiveType() const;

    //! 是否可以作为元素类型
    bool isElementType() const;

    TypeID id;                  //<! 类型 ID
    Type **containedTypes;      //<! 包含的类型
    size_t totalContainedTypes; //<! 包含的类型总数
};

struct FunctionType : public Type {
    FunctionType(Type *returnType, std::vector<Type *> &paramTypes);

    Type  *paramTypeAt(int index);
    size_t totalParams() const;

    Type *returnType; //<! 函数返回类型
};

struct SequentialType : public Type {
    SequentialType(TypeID id, Type *elementType);

    Type *elementType();
};

struct ArrayType : public SequentialType {
    ArrayType(Type *elementType, size_t length);

    size_t length; //<! 数组长度
};

struct PointerType : public SequentialType {
    PointerType(Type *elementType);
};

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
struct BasicBlock : public Value {
    BasicBlock(Function *parent, std::string_view name = "");

    Function                *parent;   //<! 所属函数
    std::list<Instruction *> instList; //<! 指令列表
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

} // namespace slime::ir

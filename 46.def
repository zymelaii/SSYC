#pragma once

namespace slime::ir {

enum class InstructionID {
    Alloca,        //<! alloca
    Load,          //<! load
    Store,         //<! store
    Ret,           //<! return
    Br,            //<! branch
    GetElementPtr, //<! getelementptr
    Add,           //<! add
    Sub,           //<! sub
    Mul,           //<! mul
    UDiv,          //<! div (unsigned)
    SDiv,          //<! div (signed)
    URem,          //<! remainder (unsigned)
    SRem,          //<! remainder (signed)
    FNeg,          //<! neg (float)
    FAdd,          //<! add (float)
    FSub,          //<! sub (float)
    FMul,          //<! mul (float)
    FDiv,          //<! div (float)
    FRem,          //<! remainder (float)
    Shl,           //<! shl
    LShr,          //<! shr (logical)
    AShr,          //<! shr (arithmetic)
    And,           //<! bitwise and
    Or,            //<! bitwise or
    Xor,           //<! xor
    FPToUI,        //<! floating point -> unsigned int
    FPToSI,        //<! floating point -> signed int
    UIToFP,        //<! unsigned int -> floating point
    SIToFP,        //<! signed int -> floating point
    ZExt,          //<! zext (i1 -> i32)
    ICmp,          //<! cmp (int)
    FCmp,          //<! cmp (float)
    Phi,           //<! Phi node instruction
    Call,          //<! call
    LAST_INST = Call,
};

enum class ComparePredicationType {
    EQ,    //<! equal
    NE,    //<! not equal
    UGT,   //<! 1. unsigned greater than (icmp)
           //<! 2. unordered or greater than (fcmp)
    UGE,   //<! 1. unsigned greater or equal (icmp)
           //<! 2. unordered or greater than or equal (fcmp)
    ULT,   //<! 1. unsigned less than (icmp)
           //<! 2. unordered or less than (fcmp)
    ULE,   //<! 1. unsigned less or equal (icmp)
           //<! 2. unordered or less than or equal (fcmp)
    SGT,   //<! signed greater than
    SGE,   //<! signed greater or equal
    SLT,   //<! signed less than
    SLE,   //<! signed less or equal
    FALSE, //<! no comparison, always returns false
    OEQ,   //<! ordered and equal
    OGT,   //<! ordered and greater than
    OGE,   //<! ordered and greater than or equal
    OLT,   //<! ordered and less than
    OLE,   //<! ordered and less than or equal
    ONE,   //<! ordered and not equal
    ORD,   //<! ordered (no nans)
    UEQ,   //<! unordered or equal
    UNE,   //<! unordered or not equal
    UNO,   //<! unordered (either nans)
    TRUE,  //<! no comparison, always returns true
};

} // namespace slime::ir

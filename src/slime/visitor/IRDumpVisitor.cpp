#include "IRDumpVisitor.h"

#include <slime/ir/user.h>
#include <slime/ir/instruction.h>
#include <array>
#include <assert.h>

namespace slime::visitor {

using namespace ir;

static inline std::ostream& operator<<(std::ostream&, std::ostream& other) {
    return other;
}

void IRDumpVisitor::dump(Module* module) {
    assert(!currentModule_);
    assert(module != nullptr);
    currentModule_ = module;
    for (auto object : *module) {
        assert(object->isGlobal());
        if (object->isFunction()) {
            dumpFunction(object->asFunction());
        } else {
            dumpGlobalVariable(object->asGlobalVariable());
        }
    }
    currentModule_ = nullptr;
}

std::ostream& IRDumpVisitor::dumpType(Type* type, bool decay) {
    switch (type->kind()) {
        case TypeKind::Void: {
            os() << "void";
        } break;
        case TypeKind::Integer: {
            os() << (type->isBoolean() ? "i1" : "i32");
        } break;
        case TypeKind::Float: {
            os() << "float";
        } break;
        case TypeKind::Pointer:
        case TypeKind::Function: {
            os() << "ptr";
        } break;
        case TypeKind::Array: {
            if (decay) {
                os() << "ptr";
            } else {
                auto array = type->asArrayType();
                os() << "[" << array->size() << " x "
                     << dumpType(array->elementType(), false) << "]";
            }
        } break;
        case TypeKind::Label: {
            os() << "label";
        } break;
        default: {
        } break;
    }
    return os();
}

std::ostream& IRDumpVisitor::dumpValueRef(Value* value) {
    assert(
        !value->isInstruction()
        || (value->asInstruction()->id() != InstructionID::Store
            && value->asInstruction()->id() != InstructionID::Br
            && value->asInstruction()->id() != InstructionID::Ret));
    if (value->isImmediate()) {
        auto imm = value->asConstantData();
        if (imm->type()->isInteger()) {
            os() << static_cast<ConstantInt*>(imm)->value;
        } else {
            os() << static_cast<ConstantFloat*>(imm)->value;
        }
    } else if (value->isGlobal()) {
        os() << "@" << value->asGlobalObject()->name();
    } else if (value->isLabel() && !value->name().empty()) {
        os() << "%" << value->name();
    } else {
        os() << "%" << value->id();
    }
    return os();
}

std::ostream& IRDumpVisitor::dumpConstant(ConstantData* data) {
    if (data->type()->isInteger()) {
        os() << "i32 " << static_cast<ConstantInt*>(data)->value;
    } else if (data->type()->isFloat()) {
        os() << "float " << static_cast<ConstantFloat*>(data)->value;
    } else {
        assert(data->type()->isArray());
        os() << dumpArrayData(static_cast<ConstantArray*>(data));
    }
    return os();
}

std::ostream& IRDumpVisitor::dumpArrayData(ConstantArray* data) {
    os() << dumpType(data->type()) << " ";
    if (data->size() == 0) {
        os() << "zeroinitializer";
    } else {
        os() << "[";
        for (int i = 0; i < data->size(); ++i) {
            os() << dumpConstant(data->at(i));
            if (i + 1 < data->size()) { os() << ", "; }
        }
        ConstantData* value = nullptr;
        const int     n     = data->type()->asArrayType()->size();
        auto          type  = data->type()->tryGetElementType();
        if (type->isArray()) {
            value = ConstantArray::create(type->asArrayType());
        } else if (type->isInteger()) {
            value = ConstantInt::create(0);
        } else if (type->isFloat()) {
            value = ConstantFloat::create(0);
        }
        assert(value != nullptr);
        for (int i = data->size(); i < n; ++i) {
            os() << ", " << dumpConstant(value);
        }
        os() << "]";
    }
    return os();
}

void IRDumpVisitor::dumpFunction(Function* func) {
    bool declareOnly = func->size() == 0;
    if (declareOnly) {
        os() << "declare ";
    } else {
        os() << "define ";
    }

    auto proto = func->proto();
    os() << dumpType(proto->returnType(), true) << " @" << func->name() << "(";
    for (int i = 0; i < proto->totalParams(); ++i) {
        os() << dumpType(proto->paramTypeAt(i), true) << " noundef "
             << dumpValueRef(const_cast<Parameter*>(func->paramAt(i)));
        if (i + 1 != proto->totalParams()) { os() << ", "; }
    }

    if (declareOnly) {
        os() << ")\n\n";
        return;
    }

    os() << ") {\n";
    for (auto block : *func) {
        if (!block->name().empty()) {
            os() << block->name() << ":\n";
        } else {
            os() << block->id() << ":\n";
        }
        for (auto inst : *block) {
            os() << "    ";
            dumpInstruction(inst);
            os() << "\n";
        }
    }
    os() << "}\n\n";
}

void IRDumpVisitor::dumpGlobalVariable(GlobalVariable* object) {
    assert(object->data() != nullptr);
    auto type = object->type()->tryGetElementType();
    assert(type->isInteger() || type->isFloat() || type->isArray());
    os() << "@" << object->name() << " = global "
         << dumpConstant(const_cast<ConstantData*>(object->data()))
         << ", align 4\n";
}

void IRDumpVisitor::dumpInstruction(Instruction* instruction) {
    static constexpr auto INST_LOOKUP = std::array<
        std::string_view,
        static_cast<size_t>(InstructionID::LAST_INST) + 1>{
        "alloca", "load", "store", "ret",  "br",     "getelementptr", "add",
        "sub",    "mul",  "udiv",  "sdiv", "urem",   "srem",          "fneg",
        "fadd",   "fsub", "fmul",  "fdiv", "frem",   "shl",           "lshr",
        "ashr",   "and",  "or",    "xor",  "fptoui", "fptosi",        "uitofp",
        "sitofp", "icmp", "fcmp",  "phi",  "call",
    };
    auto name  = INST_LOOKUP[static_cast<int>(instruction->id())];
    auto value = instruction->unwrap();
    auto type  = value->type();
    switch (instruction->id()) {
        case InstructionID::Alloca: {
            os() << dumpValueRef(value) << " = " << name << " "
                 << dumpType(type->tryGetElementType()) << ", align 4";
        } break;
        case InstructionID::Load: {
            auto inst = instruction->asLoad();
            os() << dumpValueRef(value) << " = " << name << " "
                 << dumpType(type) << ", ptr " << dumpValueRef(inst->operand())
                 << ", align 4";
        } break;
        case InstructionID::Store: {
            auto inst = instruction->asStore();
            os() << name << " " << dumpType(inst->rhs()->type()) << " "
                 << dumpValueRef(inst->rhs()) << ", ptr "
                 << dumpValueRef(inst->lhs()) << ", align 4";
        } break;
        case InstructionID::Ret: {
            auto inst = instruction->asRet();
            if (inst->operand() == nullptr) {
                os() << name << " void";
            } else {
                os() << name << " " << dumpType(inst->operand()->type(), true)
                     << " " << dumpValueRef(inst->operand());
            }
        } break;
        case InstructionID::Br: {
            auto inst = instruction->asBr();
            if (inst->op<0>()->isLabel()) {
                os() << name << " label " << dumpValueRef(inst->op<0>());
            } else {
                os() << name << " i1 " << dumpValueRef(inst->op<0>())
                     << ", label " << dumpValueRef(inst->op<1>()) << ", label "
                     << dumpValueRef(inst->op<2>());
            }
            os() << "\n";
        } break;
        case InstructionID::GetElementPtr: {
            auto inst = instruction->asGetElementPtr();
            os() << dumpValueRef(value) << " = " << name << " "
                 << dumpType(inst->lhs()->type()->tryGetElementType())
                 << ", ptr " << dumpValueRef(inst->lhs()) << ", i32 0, i32 "
                 << dumpValueRef(inst->rhs());
        } break;
        case InstructionID::Add:
        case InstructionID::Sub:
        case InstructionID::Mul:
        case InstructionID::UDiv:
        case InstructionID::SDiv:
        case InstructionID::URem:
        case InstructionID::SRem:
        case InstructionID::Shl:
        case InstructionID::LShr:
        case InstructionID::AShr:
        case InstructionID::And:
        case InstructionID::Or:
        case InstructionID::Xor: {
            auto inst = static_cast<User<2>*>(value);
            os() << dumpValueRef(value) << " = " << name << " i32 "
                 << dumpValueRef(inst->lhs()) << ", "
                 << dumpValueRef(inst->rhs());
        } break;
        case InstructionID::FNeg: {
            auto inst = instruction->asFNeg();
            os() << dumpValueRef(value) << " = " << name << " float "
                 << dumpValueRef(inst->operand());
        } break;
        case InstructionID::FAdd:
        case InstructionID::FSub:
        case InstructionID::FMul:
        case InstructionID::FDiv:
        case InstructionID::FRem: {
            auto inst = static_cast<User<2>*>(value);
            os() << dumpValueRef(value) << " = " << name << " float "
                 << dumpValueRef(inst->lhs()) << ", "
                 << dumpValueRef(inst->rhs());
        } break;
        case InstructionID::FPToUI:
        case InstructionID::FPToSI: {
            auto inst = static_cast<User<1>*>(value);
            os() << dumpValueRef(value) << " = " << name << " float "
                 << dumpValueRef(inst->operand()) << " to i32";
        } break;
        case InstructionID::UIToFP:
        case InstructionID::SIToFP: {
            auto inst = static_cast<User<1>*>(value);
            os() << dumpValueRef(value) << " = " << name << " i32 "
                 << dumpValueRef(inst->operand()) << " to float";
        } break;
        case InstructionID::ICmp: {
            auto inst = instruction->asICmp();
            os() << dumpValueRef(value) << " = " << name << " "
                 << getPredicateName(inst->predicate()) << " i32 "
                 << dumpValueRef(inst->lhs()) << ", "
                 << dumpValueRef(inst->rhs());
        } break;
        case InstructionID::FCmp: {
            auto inst = instruction->asFCmp();
            os() << dumpValueRef(value) << " = " << name << " "
                 << getPredicateName(inst->predicate()) << " float "
                 << dumpValueRef(inst->lhs()) << ", "
                 << dumpValueRef(inst->rhs());
        } break;
        case InstructionID::Phi: {
            auto inst = instruction->asPhi();
            assert(inst->totalUse() > 0 && inst->totalUse() % 2 == 0);
            os() << dumpValueRef(value) << " = " << name << " "
                 << dumpType(type) << " ";
            for (int i = 0; i < inst->totalUse(); i += 2) {
                os() << "[ " << dumpValueRef(inst->op()[i]) << ", "
                     << dumpValueRef(inst->op()[i + 1]) << " ]";
                if (i + 2 < inst->totalUse()) { os() << ", "; }
            }
        } break;
        case InstructionID::Call: {
            auto inst = instruction->asCall();
            if (!type->isVoid()) { os() << dumpValueRef(value) << " = "; }
            os() << name << " " << dumpType(type) << " "
                 << dumpValueRef(inst->callee()) << "(";
            for (int i = 0; i < inst->totalParams(); ++i) {
                auto& param = inst->paramAt(i);
                os() << dumpType(param->type(), true) << " noundef "
                     << dumpValueRef(param);
                if (i + 1 < inst->totalParams()) { os() << ", "; }
            }
            os() << ")";
        } break;
    }
}

} // namespace slime::visitor

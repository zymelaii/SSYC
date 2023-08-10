#include "98.h"

#include "51.h"
#include "47.h"
#include "84.h"
#include <array>
#include <iomanip>
#include <sstream>
#include <assert.h>

namespace slime::visitor {

using namespace ir;

static inline std::ostream& operator<<(std::ostream&, std::ostream& other) {
    return other;
}

void IRDumpVisitor::dump(Module* module) {
    assert(!currentModule_);
    assert(module != nullptr);
    pass::ValueNumberingPass{}.run(module);
    currentModule_ = module;
    os() << "; module = " << currentModule_->name() << "\n\n";
    for (auto object : module->globalObjects()) {
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
            switch (type->asIntegerType()->kind()) {
                case IntegerKind::i1: {
                    os() << "i1";
                } break;
                case IntegerKind::i8: {
                    os() << "i8";
                } break;
                case IntegerKind::i32: {
                    os() << "i32";
                } break;
            }
        } break;
        case TypeKind::Float: {
            os() << "float";
        } break;
        case TypeKind::Pointer: {
            if (testFlag(DumpOption::explicitPointerType) && !decay) {
                os() << dumpType(type->tryGetElementType()) << "*";
            } else {
                os() << "ptr";
            }
        } break;
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
        os() << dumpConstant(value->asConstantData());
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
        os() << static_cast<ConstantInt*>(data)->value;
    } else if (data->type()->isFloat()) {
        char   fp[32]{};
        double v = static_cast<ConstantFloat*>(data)->value;
        if (v == 0.) {
            strcpy(fp, "0.000000e+00");
        } else {
            sprintf(fp, "%#llx", *reinterpret_cast<uint64_t*>(&v));
        }
        os() << fp;
    } else {
        assert(data->type()->isArray());
        auto dataType = data->type()->tryGetElementType();
        auto arrType  = data->type()->asArrayType();
        auto arrData  = static_cast<ConstantArray*>(data);
        if (arrType->size() == arrData->size() && dataType->isInteger()
            && dataType->asIntegerType()->isI8()) {
            os() << "c\"";
            for (int i = 0; i < arrData->size(); ++i) {
                uint8_t ch = static_cast<ConstantInt*>(arrData->at(i))->value;
                if (ch == '\\') {
                    os() << "\\\\";
                } else if (ch >= 0x20 && ch <= 0x7e) {
                    os() << static_cast<char>(ch);
                } else {
                    os() << "\\" << std::setw(2) << std::setfill('0')
                         << std::hex << static_cast<uint32_t>(ch) << std::dec
                         << std::setfill(' ') << std::setw(1);
                }
            }
            os() << "\"";
        } else {
            os() << dumpArrayData(arrData);
        }
    }
    return os();
}

std::ostream& IRDumpVisitor::dumpArrayData(ConstantArray* data) {
    if (data->size() == 0) {
        os() << "zeroinitializer";
    } else {
        const int n        = data->type()->asArrayType()->size();
        auto      type     = data->type()->tryGetElementType();
        bool      isBottom = !type->isArray();
        os() << "[";
        for (int i = 0; i < data->size(); ++i) {
            os() << dumpType(type) << " " << dumpConstant(data->at(i));
            if (i + 1 < data->size()) { os() << ", "; }
        }
        ConstantData* value = nullptr;
        if (type->isArray()) {
            value = ConstantArray::create(type->asArrayType());
        } else if (type->isInteger()) {
            value = ConstantInt::create(0);
        } else if (type->isFloat()) {
            value = ConstantFloat::create(0);
        }
        assert(value != nullptr);
        for (int i = data->size(); i < n; ++i) {
            os() << ", " << dumpType(type) << " " << dumpConstant(value);
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
    for (auto block : func->basicBlocks()) {
        std::stringstream ss;
        if (!block->name().empty()) {
            ss << block->name() << ":";
        } else {
            ss << block->id() << ":";
        }
        os() << ss.str();
        if (block->inBlocks().size() >= 1) {
            os() << std::setw(48 - ss.str().size()) << "";
            auto it = block->inBlocks().begin();
            os() << "; preds = " << dumpValueRef(*it);
            while (++it != block->inBlocks().end()) {
                os() << ", " << dumpValueRef(*it);
            }
        }
        os() << "\n";
        for (auto inst : block->instructions()) {
            os() << "    ";
            dumpInstruction(inst);
            os() << "\n";
        }
    }
    os() << "}\n\n";
}

void IRDumpVisitor::dumpGlobalVariable(GlobalVariable* object) {
    assert(object->data() != nullptr);
    auto type          = object->type()->tryGetElementType();
    bool isI8ArrayType = false;
    if (auto dataType = type->tryGetElementType()) {
        if (auto itype = dataType->tryIntoIntegerType()) {
            isI8ArrayType = itype->isI8();
        }
    }
    assert(type->isInteger() || type->isFloat() || type->isArray());
    os() << "@" << object->name() << " = "
         << (object->isConst() ? "constant" : "global") << " "
         << dumpType(object->type()->tryGetElementType()) << " "
         << dumpConstant(const_cast<ConstantData*>(object->data()))
         << ", align " << (isI8ArrayType ? 1 : 4) << "\n\n";
}

void IRDumpVisitor::dumpInstruction(Instruction* instruction) {
    auto name  = getInstructionName(instruction);
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
                 << dumpType(type) << ", " << dumpType(inst->operand()->type())
                 << " " << dumpValueRef(inst->operand()) << ", align 4";
        } break;
        case InstructionID::Store: {
            auto inst = instruction->asStore();
            os() << name << " " << dumpType(inst->rhs()->type()) << " "
                 << dumpValueRef(inst->rhs()) << ", "
                 << dumpType(inst->lhs()->type()) << " "
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
        } break;
        case InstructionID::GetElementPtr: {
            auto inst = instruction->asGetElementPtr();
            os() << dumpValueRef(value) << " = " << name << " "
                 << dumpType(inst->op<0>()->type()->tryGetElementType()) << ", "
                 << dumpType(inst->op<0>()->type()) << " "
                 << dumpValueRef(inst->op<0>()) << ", ";
            if (inst->op<2>() != nullptr) {
                os() << "i32 " << dumpValueRef(inst->op<1>()) << ", i32 "
                     << dumpValueRef(inst->op<2>());
            } else {
                os() << "i32 " << dumpValueRef(inst->op<1>());
            }
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
        case InstructionID::ZExt: {
            auto inst = static_cast<User<1>*>(value);
            //! FIXME: handle i8
            os() << dumpValueRef(value) << " = " << name << " i1 "
                 << dumpValueRef(inst->operand()) << " to i32";
        } break;
        case InstructionID::ICmp: {
            auto inst = instruction->asICmp();
            os() << dumpValueRef(value) << " = " << name << " "
                 << getPredicateName(inst->predicate()) << " "
                 << dumpType(inst->lhs()->type()) << " "
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
            os() << name << " " << dumpType(type, true) << " "
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

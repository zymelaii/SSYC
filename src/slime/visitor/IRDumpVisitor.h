#pragma once

#include <slime/ir/module.h>
#include <slime/ir/instruction.h>
#include <map>
#include <memory>
#include <stdint.h>
#include <iostream>

namespace slime::visitor {

class IRDumpVisitor {
public:
    enum class DumpOption : uint32_t {
        explicitPointerType = 0b1,
    };

    IRDumpVisitor(const IRDumpVisitor&)            = delete;
    IRDumpVisitor& operator=(const IRDumpVisitor&) = delete;

    static IRDumpVisitor* createWithOstream(std::ostream* os) {
        return new IRDumpVisitor(os);
    }

    static inline std::string_view lookupInstName(ir::Instruction* inst) {
        return !inst ? "" : lookupInstName(inst->id());
    }

    static std::string_view lookupInstName(ir::InstructionID id);

    inline IRDumpVisitor& withOption(DumpOption opt) {
        flags_ |= static_cast<uint32_t>(opt);
        return *this;
    }

    inline bool testFlag(DumpOption flag) {
        const auto mask = static_cast<uint32_t>(flag);
        return (flags_ & mask) == mask;
    }

    void dump(ir::Module* module);

protected:
    std::ostream& dumpType(ir::Type* type, bool decay = false);
    std::ostream& dumpValueRef(ir::Value* value);
    std::ostream& dumpConstant(ir::ConstantData* data);
    std::ostream& dumpArrayData(ir::ConstantArray* data);

    void dumpFunction(ir::Function* func);
    void dumpGlobalVariable(ir::GlobalVariable* object);
    void dumpInstruction(ir::Instruction* instruction);

protected:
    IRDumpVisitor(std::ostream* os)
        : os_{os}
        , currentModule_{nullptr}
        , flags_{0} {}

    std::ostream& os() {
        return *os_;
    }

private:
    std::ostream* os_;
    ir::Module*   currentModule_;
    uint32_t      flags_;
};

} // namespace slime::visitor

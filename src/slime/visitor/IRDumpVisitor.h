#pragma once

#include <slime/ir/module.h>
#include <map>
#include <memory>
#include <iostream>

namespace slime::visitor {

class IRDumpVisitor {
public:
    IRDumpVisitor(const IRDumpVisitor&)            = delete;
    IRDumpVisitor& operator=(const IRDumpVisitor&) = delete;

    static IRDumpVisitor* createWithOstream(std::ostream* os) {
        return new IRDumpVisitor(os);
    }

    void dump(ir::Module* module);

protected:
    int idOf(ir::Value* value);

    std::ostream& dumpType(ir::Type* type, bool decay = false);
    std::ostream& dumpValueRef(ir::Value* value);

    void dumpFunction(ir::Function* func);
    void dumpGlobalVariable(ir::GlobalVariable* object);
    void dumpInstruction(ir::Instruction* instruction);

protected:
    IRDumpVisitor(std::ostream* os)
        : os_{os}
        , currentModule_{nullptr} {}

    std::ostream& os() {
        return *os_;
    }

private:
    std::ostream*             os_;
    ir::Module*               currentModule_;
    std::map<ir::Value*, int> numberingTable_;
};

} // namespace slime::visitor

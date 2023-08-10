#pragma once

#include "87.h"

namespace slime::pass {

class CopyPropagationPass : public UniversalIRPass {
public:
    //! WARNING: run after phi elimination
    void runOnFunction(ir::Function *target) override;

protected:
    void           createUseDefRecord(ir::Value *ptr);
    void           updateUseDef(ir::StoreInst *store);
    ir::Value     *lookupValueDef(ir::Value *ptr);
    ir::StoreInst *lookupLastStoreInst(ir::Value *ptr);
    bool           removeLastStoreIfUnused(ir::Value *ptr);

private:
    struct UseDefRecord {
        ir::Value     *value;
        ir::StoreInst *store;
        bool           used;
    };

    std::map<ir::Value *, UseDefRecord> useDef_;
};

} // namespace slime::pass

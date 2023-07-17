#include "CopyPropagation.h"

#include <slime/ir/instruction.h>

namespace slime::pass {

using namespace ir;

void CopyPropagationPass::runOnFunction(Function *target) {
    for (auto block : target->basicBlocks()) {
        std::map<Value*, StoreInst*> def;
        std::set<Instruction*> deleteLater;
        auto it = block->instructions().begin();
        while (it != block->instructions().end()) {
            auto inst = *it++;
            if (inst->id() == InstructionID::Store) {
                auto store = inst->asStore();
                auto &lastStore = def[store->lhs()];
                if (lastStore != nullptr) {
                    deleteLater.insert(lastStore);
                }
                lastStore = store;
                continue;
            }
            if (inst->id() == InstructionID::Load) {
                auto load = inst->asLoad();
                auto store = def[load->operand()];
                if (store != nullptr) {
                    std::vector<Use*> uses(load->uses().begin(), load->uses().end());
                    for (auto use : uses) {
                        use->reset(store->rhs());
                    }
                    deleteLater.insert(load);
                }
                continue;
            }
        }
        for (auto inst : deleteLater) {
            auto ok = inst->removeFromBlock();
            assert(ok);
        }
        //! FIXME: kill variable used by a call
    }

    return;
    for (auto block : target->basicBlocks()) {
        auto it = block->begin();
        while (it != block->end()) {
            auto inst = *it++;
            switch (inst->id()) {
                case InstructionID::Alloca: {
                    if (inst->unwrap()->uses().size() == 0) {
                        inst->removeFromBlock();
                    } else {
                        createUseDefRecord(inst->unwrap());
                    }
                    continue;
                } break;
                case InstructionID::Load: {
                    auto load = inst->asLoad();
                    auto ptr  = load->operand();
                    //! ptr may from a global object
                    if (ptr->isGlobal())
                    assert(ptr != nullptr);
                    auto &[value, store, used] = useDef_[ptr];
                    if (value == nullptr) {
                        //! WANRING: load before store
                        value = load->unwrap();
                        assert(store == nullptr);
                        used = false;
                    } else {
                        auto &uses = load->unwrap()->uses();
                        auto  it   = uses.begin();
                        while (it != uses.end()) {
                            auto use = *it++;
                            use->reset(value);
                        }
                        used = true;
                    }
                } break;
                case InstructionID::Store: {
                    updateUseDef(inst->asStore());
                    continue;
                } break;
                case InstructionID::GetElementPtr: {
                } break;
                default: {
                } break;
            }
        }
    }

    for (auto &[ptr, e] : useDef_) {
        auto &[value, store, used] = e;

        if (!used && store != nullptr) {
            store->removeFromBlock();
            store->lhs().reset();
            store->rhs().reset();
        }
        if (value && value->isInstruction()) {
            auto inst = value->asInstruction();
            if (value->uses().size() == 0) { inst->removeFromBlock(); }
        }
        //! FIXME: some bugs enables ptr to be nullptr, unexpectedly
        if (ptr && ptr->isInstruction()) {
            auto inst = ptr->asInstruction();
            if (ptr->uses().size() == 0) { inst->removeFromBlock(); }
        }
    }
    useDef_.clear();
}

void CopyPropagationPass::createUseDefRecord(Value *ptr) {
    assert(useDef_.count(ptr) == 0);
    assert(ptr != nullptr);
    auto &e = useDef_[ptr];
    e.value = nullptr;
    e.store = nullptr;
    e.used  = false;
}

void CopyPropagationPass::updateUseDef(ir::StoreInst *store) {
    auto ptr = store->lhs();
    if (useDef_.count(ptr) == 0) { return; }
    auto &[value, lastStore, used] = useDef_.at(ptr);
    assert(store != lastStore);
    removeLastStoreIfUnused(ptr);
    value     = store->rhs();
    used      = false;
    lastStore = store;
}

Value *CopyPropagationPass::lookupValueDef(Value *ptr) {
    assert(useDef_.count(ptr) == 1);
    auto &[value, store, used] = useDef_.at(ptr);
    if (!value) { return nullptr; }
    used = true;
    return value;
}

StoreInst *CopyPropagationPass::lookupLastStoreInst(Value *ptr) {
    assert(useDef_.count(ptr) == 1);
    auto &[value, store, used] = useDef_.at(ptr);
    return store;
}

bool CopyPropagationPass::removeLastStoreIfUnused(ir::Value *ptr) {
    assert(useDef_.count(ptr) == 1);
    auto &[value, store, used] = useDef_.at(ptr);
    //! value might from a before-store load inst
    if (!store) { return false; }
    if (!value) { return false; }
    if (!used || value->uses().size() == 0) {
        if (value->isInstruction()) {
            value->asInstruction()->removeFromBlock();
        }
        store->removeFromBlock();
    }
    return used == 0;
}

} // namespace slime::pass

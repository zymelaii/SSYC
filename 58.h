#include "69.h"

namespace slime::pass {

class DeadCodeEliminationPass final : public UniversalIRPass {
public:
    void run(ir::Module *module) override;
    void runOnFunction(ir::Function *target) override;

protected:
    //! address -> store
    std::map<ir::Value *, ir::StoreInst *> defList;
    //! store -> load
    std::map<ir::StoreInst *, ir::LoadInst *> useList;
};

} // namespace slime::pass
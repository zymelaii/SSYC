#include "pass.h"

namespace slime::pass {

class DeadCodeEliminationPass final : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;
};

} // namespace slime::pass

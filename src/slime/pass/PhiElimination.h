#include "pass.h"

namespace slime::pass {

class PhiEliminationPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;
};

} // namespace slime::pass

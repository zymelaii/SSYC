#include "pass.h"

namespace slime::pass {

class ResortPass : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;
};

} // namespace slime::pass

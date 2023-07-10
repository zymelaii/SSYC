#include "pass.h"

#include <set>

namespace slime::pass {

class ValueNumberingPass final : public UniversalIRPass {
public:
    void run(ir::Module *target) override;
    void runOnFunction(ir::Function *target) override;

private:
    int                   nextId_ = 0;
    std::set<ir::Value *> doneSet_;
};

} // namespace slime::pass
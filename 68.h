#include "69.h"

#include <set>

namespace slime::pass {

class ValueNumberingPass final : public UniversalIRPass {
public:
    void runOnFunction(ir::Function *target) override;

private:
    int                   nextId_ = 0;
    std::set<ir::Value *> doneSet_;
};

} // namespace slime::pass
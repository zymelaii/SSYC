#include "85.h"

#include <set>

namespace slime::pass {

class ValueNumberingPass final : public UniversalIRPass {
public:
    inline void run(ir::Module *target) override;
    void        runOnFunction(ir::Function *target) override;

private:
    int                   nextId_ = 0;
    std::set<ir::Value *> doneSet_;
};

inline void ValueNumberingPass::run(ir::Module *target) {
    for (auto obj : target->globalObjects()) {
        if (obj->isFunction()) { runOnFunction(obj->asFunction()); }
    }
}

} // namespace slime::pass
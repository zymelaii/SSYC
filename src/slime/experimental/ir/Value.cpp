#include "Value.h"

#include <vector>
#include <algorithm>

namespace slime::experimental::ir {

void ValueBaseImpl::addUse(Use *use) const {
    for (auto e : uses()) {
        if (e == use) { return; }
    }
    useList_.insertToTail(use);
}

void ValueBaseImpl::removeUse(Use *use) const {
    if (auto it = uses().find(use); it != uses().node_end()) {
        it->removeFromList();
    }
}

} // namespace slime::experimental::ir

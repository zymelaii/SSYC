#include "use.h"
#include "value.h"

namespace slime::ir {

void Use::set(Value *value) {
    if (value_) {
        //! TODO: remove from use list
    }
    value_ = value;
    if (value_ != nullptr) {
        //! TODO: add use of the value
    }
}

Value *Use::operator=(Value *rhs) {
    set(rhs);
    return rhs;
}

const Use &Use::operator=(const Use &rhs) {
    set(rhs.value_);
    return *this;
}

} // namespace slime::ir

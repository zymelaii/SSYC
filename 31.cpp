#include "32.h"

namespace slime::experimental::ir {

UserBase::FnPtrTable<&UserBase::used> UserBase::used_FNPTR_TABLE{
    &UserBase::usedForwardFn<SpecificUser<-1>>,
    &UserBase::usedForwardFn<SpecificUser<0>>,
    &UserBase::usedForwardFn<SpecificUser<1>>,
    &UserBase::usedForwardFn<SpecificUser<2>>,
    &UserBase::usedForwardFn<SpecificUser<3>>,
};

UserBase::FnPtrTable<&UserBase::totalUse> UserBase::totalUse_FNPTR_TABLE{
    &UserBase::totalUseForwardFn<SpecificUser<-1>>,
    &UserBase::totalUseForwardFn<SpecificUser<0>>,
    &UserBase::totalUseForwardFn<SpecificUser<1>>,
    &UserBase::totalUseForwardFn<SpecificUser<2>>,
    &UserBase::totalUseForwardFn<SpecificUser<3>>,
};

UserBase::FnPtrTable<&UserBase::uses> UserBase::uses_FNPTR_TABLE{
    &UserBase::usesForwardFn<SpecificUser<-1>>,
    &UserBase::usesForwardFn<SpecificUser<0>>,
    &UserBase::usesForwardFn<SpecificUser<1>>,
    &UserBase::usesForwardFn<SpecificUser<2>>,
    &UserBase::usesForwardFn<SpecificUser<3>>,
};

} // namespace slime::experimental::ir
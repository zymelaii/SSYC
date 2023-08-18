#pragma once

#include "../Utility.h"
#include "../utils/TryIntoTrait.h"

namespace slime::lang {
inline namespace type {

class MetaTy;      //<! type
class VoidTy;      //<! void type
class IntTy;       //<! integer type
class FPTy;        //<! IEEE 754 floating-point type
class BoolTy;      //<! boolean type
class ByteTy;      //<! byte type
class CharTy;      //<! charater type
class PtrTy;       //<! pointer type
class ArrayTy;     //<! array type
class FnTy;        //<! function proto type
class StructTy;    //<! combination type
class EnumTy;      //<! enumeration type
class EnumClassTy; //<! enumeration class type
class AliasTy;     //<! alias type

} // namespace type
} // namespace slime::lang

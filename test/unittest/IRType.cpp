#include <gtest/gtest.h>
#include <slime/experimental/ir/Type.h>

using namespace slime::experimental::ir;

TEST(IRType, PrimaryTypeSignature) {
    ASSERT_EQ(Type::getVoidTy()->signature(), "void");

    ASSERT_EQ(Type::getBoolTy()->signature(), "bool");
    ASSERT_EQ(Type::getUPtrTy()->signature(), "addr");
    ASSERT_EQ(Type::getI8Ty()->signature(), "i8");
    ASSERT_EQ(Type::getU8Ty()->signature(), "u8");
    ASSERT_EQ(Type::getI16Ty()->signature(), "i16");
    ASSERT_EQ(Type::getU16Ty()->signature(), "u16");
    ASSERT_EQ(Type::getI32Ty()->signature(), "i32");
    ASSERT_EQ(Type::getU32Ty()->signature(), "u32");
    ASSERT_EQ(Type::getI64Ty()->signature(), "i64");
    ASSERT_EQ(Type::getU64Ty()->signature(), "u64");

    ASSERT_EQ(Type::getFP32Ty()->signature(), "f32");
    ASSERT_EQ(Type::getFP64Ty()->signature(), "f64");
    ASSERT_EQ(Type::getFP128Ty()->signature(), "f128");
}

TEST(IRType, PtrTypeSignature) {
    ASSERT_EQ(Type::getPtrTy(Type::getVoidTy())->signature(), "Pvoid");
    ASSERT_EQ(Type::getPtrTy(Type::getBoolTy())->signature(), "Pbool");
    ASSERT_EQ(Type::getPtrTy(Type::getUPtrTy())->signature(), "Paddr");
    ASSERT_EQ(Type::getPtrTy(Type::getI8Ty())->signature(), "Pi8");
    ASSERT_EQ(Type::getPtrTy(Type::getU8Ty())->signature(), "Pu8");
    ASSERT_EQ(Type::getPtrTy(Type::getI16Ty())->signature(), "Pi16");
    ASSERT_EQ(Type::getPtrTy(Type::getU16Ty())->signature(), "Pu16");
    ASSERT_EQ(Type::getPtrTy(Type::getI32Ty())->signature(), "Pi32");
    ASSERT_EQ(Type::getPtrTy(Type::getU32Ty())->signature(), "Pu32");
    ASSERT_EQ(Type::getPtrTy(Type::getI64Ty())->signature(), "Pi64");
    ASSERT_EQ(Type::getPtrTy(Type::getU64Ty())->signature(), "Pu64");
    ASSERT_EQ(Type::getPtrTy(Type::getFP32Ty())->signature(), "Pf32");
    ASSERT_EQ(Type::getPtrTy(Type::getFP64Ty())->signature(), "Pf64");
    ASSERT_EQ(Type::getPtrTy(Type::getFP128Ty())->signature(), "Pf128");

    auto pv = Type::getPtrTy(Type::getVoidTy());
    ASSERT_EQ(Type::getPtrTy(pv)->signature(), "PPvoid");
    ASSERT_EQ(Type::getPtrTy(Type::getPtrTy(pv))->signature(), "PPPvoid");

    auto i8  = Type::getI8Ty();
    auto a0  = Type::getArrayTy(i8, 0);
    auto a32 = Type::getArrayTy(i8, 32);
    ASSERT_EQ(Type::getPtrTy(a0)->signature(), "PA0_i8");
    ASSERT_EQ(Type::getPtrTy(a32)->signature(), "PA32_i8");
    ASSERT_EQ(Type::getPtrTy(Type::getPtrTy(a0))->signature(), "PPA0_i8");
    ASSERT_EQ(Type::getPtrTy(Type::getPtrTy(a32))->signature(), "PPA32_i8");

    auto f0  = Type::getFnTy(false, i8);
    auto fv0 = Type::getFnTy(true, i8);
    auto f4  = Type::getFnTy(false, i8, pv, f0, a32, i8);
    ASSERT_EQ(Type::getPtrTy(f0)->signature(), "PFi8T");
    ASSERT_EQ(Type::getPtrTy(fv0)->signature(), "PFVi8T");
    ASSERT_EQ(Type::getPtrTy(f4)->signature(), "PFi8_PvoidFi8TA32_i8i8T");
}

TEST(IRType, ArrayTypeSignature) {
    auto i8       = Type::getI8Ty();
    auto a1_2_3_4 = Type::getArrayTy(
        Type::getArrayTy(Type::getArrayTy(Type::getArrayTy(i8, 4), 3), 2), 1);

    ASSERT_EQ(Type::getArrayTy(i8, 0)->signature(), "A0_i8");
    ASSERT_EQ(Type::getArrayTy(i8, 32)->signature(), "A32_i8");
    ASSERT_EQ(Type::getArrayTy(Type::getPtrTy(i8), 32)->signature(), "A32_Pi8");
    ASSERT_EQ(a1_2_3_4->signature(), "A1_2_3_4_i8");

    auto f4 = Type::getFnTy(
        false,
        i8,
        Type::getPtrTy(Type::getVoidTy()),
        Type::getFnTy(false, i8),
        Type::getArrayTy(Type::getPtrTy(i8), 32),
        i8);
    ASSERT_EQ(
        Type::getArrayTy(f4, 0)->signature(), "A0_Fi8_PvoidFi8TA32_Pi8i8T");
    ASSERT_EQ(
        Type::getArrayTy(Type::getArrayTy(f4, 0), 999)->signature(),
        "A999_0_Fi8_PvoidFi8TA32_Pi8i8T");
}

TEST(IRType, FnProtoTypeSignature) {
    auto i8  = Type::getI8Ty();
    auto f0  = Type::getFnTy(false, i8);
    auto f1  = Type::getFnTy(false, i8, i8);
    auto fv1 = Type::getFnTy(true, i8, i8);
    auto fv0 = Type::getFnTy(true, i8);
    auto f4  = Type::getFnTy(
        false,
        i8,
        Type::getPtrTy(Type::getVoidTy()),
        Type::getFnTy(false, i8),
        Type::getArrayTy(Type::getPtrTy(i8), 32),
        i8);
    auto fv4 = Type::getFnTy(
        true,
        i8,
        Type::getPtrTy(Type::getVoidTy()),
        Type::getFnTy(false, i8),
        Type::getArrayTy(Type::getPtrTy(i8), 32),
        i8);
    auto ff0  = Type::getFnTy(false, fv4);
    auto ffv0 = Type::getFnTy(true, fv4);
    auto ff1  = Type::getFnTy(false, fv4, i8);
    auto ffv1 = Type::getFnTy(true, fv4, i8);

    ASSERT_EQ(f0->signature(), "Fi8T");
    ASSERT_EQ(f1->signature(), "Fi8_i8T");
    ASSERT_EQ(fv1->signature(), "FVi8_i8T");
    ASSERT_EQ(fv0->signature(), "FVi8T");
    ASSERT_EQ(f4->signature(), "Fi8_PvoidFi8TA32_Pi8i8T");
    ASSERT_EQ(fv4->signature(), "FVi8_PvoidFi8TA32_Pi8i8T");
    ASSERT_EQ(ff0->signature(), "FFVi8_PvoidFi8TA32_Pi8i8TT");
    ASSERT_EQ(ffv0->signature(), "FVFVi8_PvoidFi8TA32_Pi8i8TT");
    ASSERT_EQ(ff1->signature(), "FFVi8_PvoidFi8TA32_Pi8i8T_i8T");
    ASSERT_EQ(ffv1->signature(), "FVFVi8_PvoidFi8TA32_Pi8i8T_i8T");
}

TEST(IRType, FromMethod) {
    ASSERT_EQ(Type::from("void")->signature(), "void");
    ASSERT_EQ(Type::from("bool")->signature(), "bool");
    ASSERT_EQ(Type::from("addr")->signature(), "addr");
    ASSERT_EQ(Type::from("i8")->signature(), "i8");
    ASSERT_EQ(Type::from("u8")->signature(), "u8");
    ASSERT_EQ(Type::from("i16")->signature(), "i16");
    ASSERT_EQ(Type::from("u16")->signature(), "u16");
    ASSERT_EQ(Type::from("i32")->signature(), "i32");
    ASSERT_EQ(Type::from("u32")->signature(), "u32");
    ASSERT_EQ(Type::from("i64")->signature(), "i64");
    ASSERT_EQ(Type::from("u64")->signature(), "u64");
    ASSERT_EQ(Type::from("f32")->signature(), "f32");
    ASSERT_EQ(Type::from("f64")->signature(), "f64");
    ASSERT_EQ(Type::from("f128")->signature(), "f128");

    ASSERT_EQ(Type::from("Pvoid")->signature(), "Pvoid");
    ASSERT_EQ(Type::from("Pbool")->signature(), "Pbool");
    ASSERT_EQ(Type::from("Paddr")->signature(), "Paddr");
    ASSERT_EQ(Type::from("Pi8")->signature(), "Pi8");
    ASSERT_EQ(Type::from("Pu8")->signature(), "Pu8");
    ASSERT_EQ(Type::from("Pi16")->signature(), "Pi16");
    ASSERT_EQ(Type::from("Pu16")->signature(), "Pu16");
    ASSERT_EQ(Type::from("Pi32")->signature(), "Pi32");
    ASSERT_EQ(Type::from("Pu32")->signature(), "Pu32");
    ASSERT_EQ(Type::from("Pi64")->signature(), "Pi64");
    ASSERT_EQ(Type::from("Pu64")->signature(), "Pu64");
    ASSERT_EQ(Type::from("Pf32")->signature(), "Pf32");
    ASSERT_EQ(Type::from("Pf64")->signature(), "Pf64");
    ASSERT_EQ(Type::from("Pf128")->signature(), "Pf128");
    ASSERT_EQ(Type::from("PPvoid")->signature(), "PPvoid");
    ASSERT_EQ(Type::from("PPPvoid")->signature(), "PPPvoid");
    ASSERT_EQ(Type::from("PA0_i8")->signature(), "PA0_i8");
    ASSERT_EQ(Type::from("PA32_i8")->signature(), "PA32_i8");
    ASSERT_EQ(Type::from("PPA0_i8")->signature(), "PPA0_i8");
    ASSERT_EQ(Type::from("PPA32_i8")->signature(), "PPA32_i8");
    ASSERT_EQ(Type::from("PFi8T")->signature(), "PFi8T");
    ASSERT_EQ(Type::from("PFVi8T")->signature(), "PFVi8T");
    ASSERT_EQ(
        Type::from("PFi8_PvoidFi8TA32_i8i8T")->signature(),
        "PFi8_PvoidFi8TA32_i8i8T");

    ASSERT_EQ(Type::from("A0_i8")->signature(), "A0_i8");
    ASSERT_EQ(Type::from("A32_i8")->signature(), "A32_i8");
    ASSERT_EQ(Type::from("A32_Pi8")->signature(), "A32_Pi8");
    ASSERT_EQ(Type::from("A1_2_3_4_i8")->signature(), "A1_2_3_4_i8");
    ASSERT_EQ(
        Type::from("A0_Fi8_PvoidFi8TA32_Pi8i8T")->signature(),
        "A0_Fi8_PvoidFi8TA32_Pi8i8T");
    ASSERT_EQ(
        Type::from("A999_0_Fi8_PvoidFi8TA32_Pi8i8T")->signature(),
        "A999_0_Fi8_PvoidFi8TA32_Pi8i8T");

    ASSERT_EQ(Type::from("Fi8T")->signature(), "Fi8T");
    ASSERT_EQ(Type::from("Fi8_i8T")->signature(), "Fi8_i8T");
    ASSERT_EQ(Type::from("FVi8_i8T")->signature(), "FVi8_i8T");
    ASSERT_EQ(Type::from("FVi8T")->signature(), "FVi8T");
    ASSERT_EQ(
        Type::from("Fi8_PvoidFi8TA32_Pi8i8T")->signature(),
        "Fi8_PvoidFi8TA32_Pi8i8T");
    ASSERT_EQ(
        Type::from("FVi8_PvoidFi8TA32_Pi8i8T")->signature(),
        "FVi8_PvoidFi8TA32_Pi8i8T");
    ASSERT_EQ(
        Type::from("FFVi8_PvoidFi8TA32_Pi8i8TT")->signature(),
        "FFVi8_PvoidFi8TA32_Pi8i8TT");
    ASSERT_EQ(
        Type::from("FVFVi8_PvoidFi8TA32_Pi8i8TT")->signature(),
        "FVFVi8_PvoidFi8TA32_Pi8i8TT");
    ASSERT_EQ(
        Type::from("FFVi8_PvoidFi8TA32_Pi8i8T_i8T")->signature(),
        "FFVi8_PvoidFi8TA32_Pi8i8T_i8T");
    ASSERT_EQ(
        Type::from("FVFVi8_PvoidFi8TA32_Pi8i8T_i8T")->signature(),
        "FVFVi8_PvoidFi8TA32_Pi8i8T_i8T");
}

#include <gtest/gtest.h>
#include <slime/ir/instruction.h>
#include <slime/ir/user.h>

using namespace slime::ir;

TEST(IRInstruction, PatchedTotalOperands) {
    auto i1  = ConstantData::getBoolean(false);
    auto i32 = ConstantInt::create(0);
    auto f32 = ConstantFloat::create(0.f);
    auto fn = Function::create("fn", FunctionType::create(Type::getVoidType()));
    auto fni = Function::create(
        "fni",
        FunctionType::create(Type::getVoidType(), Type::getIntegerType()));
    auto block            = BasicBlock::create(fn);
    auto pointer          = Instruction::createAlloca(Type::getIntegerType());
    auto addressToPointer = Instruction::createAlloca(
        Type::createPointerType(Type::getIntegerType()));
    auto adderssToArray = Instruction::createAlloca(
        Type::createArrayType(Type::getIntegerType(), 8));
    auto phi       = Instruction::createPhi(Type::getIntegerType());
    auto predicate = ComparePredicationType::EQ;

    EXPECT_TRUE(pointer->insertToHead(block));

    ASSERT_EQ(pointer->totalOperands(), 0);
    ASSERT_EQ(Instruction::createLoad(pointer)->totalOperands(), 1);
    ASSERT_EQ(Instruction::createStore(pointer, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createRet()->totalOperands(), 1);
    ASSERT_EQ(Instruction::createRet(i32)->totalOperands(), 1);
    ASSERT_EQ(Instruction::createBr(block)->totalOperands(), 3);
    ASSERT_EQ(Instruction::createBr(i1, block, block)->totalOperands(), 3);

    ASSERT_EQ(
        Instruction::createGetElementPtr(pointer, i32)->totalOperands(), 3);
    ASSERT_EQ(
        Instruction::createGetElementPtr(adderssToArray, i32, i32)
            ->totalOperands(),
        3);

    ASSERT_EQ(Instruction::createAdd(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createSub(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createMul(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createUDiv(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createSDiv(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createURem(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createSRem(i32, i32)->totalOperands(), 2);

    ASSERT_EQ(Instruction::createFNeg(f32)->totalOperands(), 1);
    ASSERT_EQ(Instruction::createFAdd(f32, f32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createFSub(f32, f32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createFMul(f32, f32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createFDiv(f32, f32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createFRem(f32, f32)->totalOperands(), 2);

    ASSERT_EQ(Instruction::createShl(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createLShr(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createAShr(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createAnd(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createOr(i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createXor(i32, i32)->totalOperands(), 2);

    ASSERT_EQ(Instruction::createFPToUI(f32)->totalOperands(), 1);
    ASSERT_EQ(Instruction::createFPToSI(f32)->totalOperands(), 1);
    ASSERT_EQ(Instruction::createUIToFP(i32)->totalOperands(), 1);
    ASSERT_EQ(Instruction::createSIToFP(i32)->totalOperands(), 1);

    ASSERT_EQ(Instruction::createZExt(i1)->totalOperands(), 1);

    ASSERT_EQ(Instruction::createICmp(predicate, i32, i32)->totalOperands(), 2);
    ASSERT_EQ(Instruction::createFCmp(predicate, f32, f32)->totalOperands(), 2);

    ASSERT_EQ(phi->totalOperands(), 0);
    EXPECT_TRUE(phi->addIncomingValue(i32, block));
    ASSERT_EQ(phi->totalOperands(), 2)
        << phi->totalOperands() << " total use in phi " << phi;
    EXPECT_TRUE(phi->removeValueFrom(block));
    ASSERT_EQ(phi->totalOperands(), 2);

    ASSERT_EQ(Instruction::createCall(fn)->totalOperands(), 1);
    ASSERT_EQ(Instruction::createCall(fni)->totalOperands(), 2);
}

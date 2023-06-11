# 前端

## 选型

- SysY 2022 文法的超集
- 自定义的 Lexer
- 选用 AST 作为第一级 IR
- 基于递归下降分析法 LL(1) 文法自定义的 Parser
- 选用 LLVM-IR 子集作为 IR Code 的格式

## TODO List

- [ ] Lexer
- [ ] Parser
- [ ] AST 生成
- [ ] IR 生成

# 机器无关优化

## 选型

- 选用 CFG 辅助数据流分析
- 采用部分 SSA 的 IR 格式

## TODO List

### 基本项

- [ ] CFG 生成
- [ ] SSA 结构生成
  - [ ] 支配树生成
  - [ ] 汇合边分析
  - [ ] 支配边界构建
  - [ ] Phi 指令处理
  - [ ] 变量重命名

### 优化项

主要任务：

- [ ] 常量传播与折叠 (Constant Propagation and Folding)
  - [ ] `PASS` 常量传播 (Constant Propagation)
  - [ ] `PASS` 常数折叠 (Constant Folding)
- [ ] `PASS` 公共子表达式消除 (Common Subexpression Elimination)
- [ ] `PASS` 死代码消除 (Dead-code Elimination)
- [ ] `PASS` 全局值编号 (Global Value Numbering)
- [ ] `PASS` 无效变量消除 (Dead-store Elimination)
- [ ] `PASS` 复制传播 (Copy Propagation)
- [ ] `PASS` 尾调用优化 (Tail-call Optimization)
- [ ] 归纳变量识别与消除 (Induction Variable Recognition and Elimination)
  - [ ] 归纳变量识别 (Induction Variable Recognition)
  - [ ] `PASS` 归纳变量消除 (Induction Variable Elimination)
- [ ] `PASS` 循环反转 (Loop Inversion)
- [ ] `PASS` 循环展开 (Loop Unrolling)

可选项：

- [ ] `PASS` 内联展开 (Inline Expansion)
- [ ] `PASS` 边界检查消除 (Bounds-checking Elimination)
- [ ] `PASS` 循环嵌套优化 (Loop Nest Optimization)
- [ ] `PASS` 循环不变量移动 (Loop-invariant Code Motion)

# 后端

## 选型

- 目标机器架构 ARM-v7 32-bit
- 选用图着色算法进行寄存器分配

## TODO List

### 基本项

- [ ] 指令指派
- [ ] 寄存器分配

### 优化项

- [ ] `PASS` 窥孔优化 (Peephole Optimizations)

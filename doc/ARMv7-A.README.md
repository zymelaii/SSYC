# 关于 ARMv7-A 后端的一些说明

1. 选定 ISA 为 ARMv7-A
2. 选定 FPU 为 VFPv3 `-mfpu=vfpv3`
3. 启用 Thumb-2 拓展
4. 使用硬件实现的浮点指令 `-mfpu-abi=hard`
5. 使用 [unified 语法](https://sourceware.org/binutils/docs/as/ARM_002dInstruction_002dSet.html)
6. 汇编格式参照 GNU，保留部分差异
7. 由于 thumb 指令的启用，代码段必须对齐到 4 字节以确保 PC 计算的正确性（计算常量池偏移时使用）

更多信息见[关于 ARM 汇编指示的说明](https://sourceware.org/binutils/docs/as/ARM-Directives.html)。

# 全局字符串池

维护全过程的字符串池

1. 字符串内容的唯一性

不存在相同内容的字符串

2. 独占内存

字符串的内存由字符串值独占

3. 字符串的复用

对于相同后缀的字符串进行复用

父串基址的前一个字节置 0 以作标记

# COW Value Wrapper

在 IR 阶段，关于 Value 的分配使用 Value Wrapper 替代 Value，仅在 Value Wrapper 记录最少的差异信息，当且仅当额外信息被写入时创建新 Value 并重置 Wrapper。

# 立即数池

用于 IR 及 ASM 阶段

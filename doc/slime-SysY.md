# Features

- [ ] [unified ABI](#unified-abi)

- [x] [builtin preprocessor](#builtin-preprocessor)

- [ ] [keyword decltype](#keyword-decltype)
- [ ] [keyword typeof](#keyword-typeof)
- [ ] [keyword sizeof](#keyword-sizeof)

- [x] [builtin type char](#builtin-type-char)
- [ ] [builtin type byte](#builtin-type-byte)
- [ ] [builtin type short](#builtin-type-short)
- [x] [builtin type int](#builtin-type-int)
- [ ] [builtin type long](#builtin-type-long)
- [ ] [builtin type long long](#builtin-type-long-long)
- [x] [builtin type float](#builtin-type-float)
- [ ] [builtin type double](#builtin-type-double)
- [ ] [builtin type long double](#builtin-type-long-double)

- [ ] [struct type](#struct-type)
- [ ] [enum type](#enum-type)
- [ ] [enum class type](#enum-class-type)

- [x] [declarator const](#declarator-const)
- [x] [declarator extern](#declarator-extern)
- [x] [declarator static](#declarator-static)
- [x] [declarator inline](#declarator-inline)
- [ ] [declarator constexpr](#declarator-constexpr)

- [ ] [attributable keyword](#attributable-keyword)
- [ ] [attribute force for inline](#attribute-force-for-inline)
- [ ] [attribute compile-time for constexpr](#attribute-compile-time-for-constexpr)

- [x] [integer literal support](#integer-literal-support)
- [x] [char literal support](#char-literal-support)
- [x] [floating-point literal support](#floating-point-literal-support)
- [x] [string literal support](#string-literal-support)
- [ ] [raw string literal support](#raw-string-literal-support)
- [ ] [formattable string literal support](#formattable-string-literal-support)

- [ ] [high-precision arithmetic constexpr evaluation](#high-precision-arithmetic-constexpr-evaluation)

- [ ] [strong-typed literal](#strong-typed-literal)
- [ ] [variable-length array](#variable-length-array)
- [ ] [string literals splice](#string-literals-splice)
- [ ] [force utf-8 encoding](#force-utf-8-encoding)

# Definition

## Unified ABI

## Builtin Preprocessor

支持源码的预处理。

支持预定义宏的替换。

## Keyword `decltype`

关键字 `decltype` 用于定义类型别名与类型计算

类型别名格式：`decltype <alias-type> = <compound-type>;`

类型计算格式：`decltype(<type-expression>);`

## Keyword `typeof`

关键字 `typeof` 用于获取值的类型

格式：`typeof(<expression>)`

## Keyword `sizeof`

关键字 `sizeof` 用于获取目标类型的内存大小（单位为字节）

格式：`sizeof(<type-or-expression>)`

## Builtin Type `char`

## Builtin Type `byte`

## Builtin Type `short`

## Builtin Type `int`

## Builtin Type `long`

## Builtin Type `long long`

## Builtin Type `float`

## Builtin Type `double`

## Builtin Type `long double`

## `struct` Type

使用关键字 `struct` 定义结构体类型

结构体类型是类型的顺序组合

## `enum` Type

使用关键字 `enum` 定义枚举类型

枚举类型必须指定底层值类型（整型）

可以手动指定枚举项的值，未指定时，枚举项的值将递增

当存在值相同的枚举项时，给出警告

枚举类型可以隐式转换为指定的底层值类型

参与显式转换时，枚举类型将按底层值类型参与转换

无法从任意类型转换到枚举类型，也即枚举值是安全的

示例：

```plain
enum Color : int {
    Red = 0,
    Blue,
    Green,
    Pink = 114,
    Gray,
    Orange,
};
```

## `enum class` Type

使用关键字 `enum class` 定义枚举类类型

枚举类支持枚举项的自定义类型

示例：

```plain
enum class User {
    Guest,
    Default(char*),
    Admin { access: int, name: char* },
};
```

## Declarator `const`

## Declarator `extern`

## Declarator `static`

## Declarator `inline`

支持函数内联。

被 `inline` 修饰的函数将根据具体情况尝试内联。

## Declarator `constexpr`

支持显式编译期常量的定义。

被 `constexpr` 限定的变量将强制升为编译期常量，若初值为非编译期常量，抛出错误。

被 `constexpr` 限定的函数将根据传入参数尝试编译期求值，若函数是非编译期可求值的，抛出错误；若参数是编译期常量，进行编译期求值；若参数包含非编译期常量，进行正常的函数调用。

`constexpr` 限定的函数签名与非 `constexpr` 版本一致。

可以用 `constexpr` 限定函数调用，被 `constexpr` 修饰的函数调用将强制进行编译期求值，若失败则抛出错误。

## Attributable Keyword

支持关键字的属性配置。

格式：`keyword(attribute, ...)`

## Attribute `force` for `inline`

为 `inline` 添加 `force` 属性，用于指定强制启用函数内联。

## Attribute `compile-time` for `constexpr`

为 `constexpr` 添加 `compile-time` 属性，强制函数升为编译期。

若修饰变量，抛出警告并忽略。

若函数编译期不可求值，抛出错误。

若存在函数的调用处不可编译期求值，抛出错误。

`constexpr(inline)` 限定的函数签名与非限定的版本不一致。

当存在符号冲突时，函数选取的判决顺序为非编译期函数、不具 `compile-time` 属性的编译期求值函数、具备 `compile-time` 属性的编译期求值函数。

可以使用 `constexpr` 显式指定调用编译期函数，函数选取的判决顺序为具备 `compile-time` 属性的编译期求值函数、不具 `compile-time` 属性的编译期求值函数。

示例：

```plain
int f(int x) { return x + 3; }
// constexpr int f(int x) { return x + 2; }            //<! error
constexpr(compile-time) int f(int x) { return x + 1; }

constexpr int g(int x) { return x + 3; }
constexpr(inline) g(int x) { return x + 2; }

int main() {
    int a = 3;

    f(3);              //<! 6
    constexpr f(3);    //<! 4

    g(3);              //<! 6
    constexpr g(3);    //<! 5
    g(a);              //<! 6
    // constexpr g(a); //<! error
}
```

## Integer Literal Support

支持二进制、八进制、十进制、十六进制的整型字面量。

允许使用 `_` 作为 digits 部分的分隔符。

## Char Literal Support

支持字符型常量。

若给定的字符型为空，抛出警告并置为 '\0'。

若给定的字符型多于一个，抛出警告并抛弃多余的字符。

格式：`'<single-character>'`

## Floating-point Literal Support

支持小数表示法、科学计数法、十六进制表示法的浮点数字面量。

## String Literal Support

支持字符串字面量。

格式：`"<your-string>"`

## Raw String Literal Support

支持非转义字符串字面量。

格式：`R"(<your-raw-string>)"`。

## Formattable String Literal Support

支持可格式化字符串字面量，允许使用编译期常量对字符串进行子表达式替换。

格式：`$"{compile-time-constant}..."`

示例：

```plain
constexpr char* a = "Hello";
constexpr float b = 3.2;
constexpr int   c = -123;
$"Hello World!";                //<! "Hello World!"
$"{s} World{'!'}";              //<! "Hello World!"
$"{s} World{'!'} --> {b + c}";  //<! "Hello World! --> -119.8"
$"\{{c}\{\}";                   //<! "{-123{}"
```

## High-precision Arithmetic `constexpr` Evaluation

添加 `s` 用以标记算术字面量为高精度值，`s` 后缀必须位于所有后缀的最末尾。

被标记为高精度值的编译期常量参与编译期算术运算时将以真值进行计算。

在非编译期算术运算时，以转换后对应的类型值参与计算。

高精度值标记具有传递性。

显式类型转换将消除高精度值标记。

编译期函数调用执行高精度计算当且仅当 `constexpr` 具备 `force` 属性且相关计算存在高精度值标记。

## Strong-typed Literal

字面量常量拥有唯一确定的类型。

## Variable-length Array

支持使用非常量整型变量定义数组长度。

对于有符号整型，判断值的非负性，若期望长度小于零，则分配失败。

使用无符号整型可以消除非负性的判断。

对于 16 位及以下的整型，内存从栈上获取。

对于 32 位及以上的整形，判断长度的有效性，若栈空间不足则将尝试从堆申请内存，否则从栈上分配空间。

栈上分配失败时，程序的行为是不确定的；在其它情况下，分配失败将转移到内置异常处理函数并退出。

VLA 的生命周期与常规定长数组保持一致，声明周期结束将释放内存。

VLA 的长度在确定后不可变。

## String Literals Splice

支持连续字符串编译期常量的合并。

其中字符串编译期常量包括字符串字面量与 `constexpr` 限定的字符串型变量。

## Force UTF-8 Encoding

强制以 UTF-8 编码解析源文件。

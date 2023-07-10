# Slime 使用教程

## AST

### Decl | 创建一个声明

现阶段所有的 Decl 都是 NamedDecl，要求必须通过以下方法构造：

```cpp
//! 1. 获取 DeclSpecifier 实例
auto specifier = DeclSpecifier::create();
//! 2. 处理 specifier 的属性
//! 3. 从 specifier 构造 NamedDecl
//! 3-1. 构造 VarDecl，以下代码将构造一个默认未初始化的 VarDecl
std::string_view varName = getVarNameViaYourMethod();
auto var = specifier->createVarDecl(varName);
//! 3-2. 构造 FunctionDecl
std::string_view funcName = getFuncNameViaYourMethod();
ParamVarDeclList params = getParamListViaYourMethod();
CompoundStmt *body = getFuncBodyViaYourMethod();
auto func = specifier->createFunctionDecl(funcName, params, body);
```

注意事项：

- specifier 由任意个 NamedDecl 持有，除非你清楚自己在做什么，禁止手动析构
- 不建议在 NamedDecl 构造完成之后修改 specifier 的属性
- 若必要修改 specifier 的属性且 specifier 被多个 NamedDecl 持有，建议使用 clone 方法获取一份拷贝
- specifier 的 type 域指定了 NamedDecl 的类型，如果二者不对等，需手动纠正

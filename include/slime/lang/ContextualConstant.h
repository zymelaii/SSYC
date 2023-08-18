#pragma once

namespace slime::lang {

enum class ContextualConstantKind {
    File,     //<! current file source
    Line,     //<! current line
    Column,   //<! current column
    Scope,    //<! current scope
    Function, //<! current function
    Date,     //<! compilation date
    Time,     //<! compilation time
    Version,  //<! slime-SysY version
};

} // namespace slime::lang

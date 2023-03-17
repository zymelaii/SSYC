#pragma once

#include "parser_context.h"

#include <stdio.h>
#include <optional>
#include <memory>
#include <string_view>
#include <filesystem>

namespace ssyc {

class Parser {
public:
    Parser();

public:
    bool           setSource(std::string_view sourcePath) noexcept;
    bool           execute();
    ParserContext* context();

private:
    std::optional<FILE*>           optInStream;
    std::unique_ptr<ParserContext> ptrContext;
};

} // namespace ssyc

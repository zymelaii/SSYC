#pragma once

#include <stdio.h>
#include <optional>
#include <string_view>
#include <filesystem>

namespace ssyc {

class Parser {
public:
    Parser() = default;

public:
    bool setSource(std::string_view sourcePath) noexcept;
    bool execute();

private:
    std::optional<FILE *> optInStream;
};

} // namespace ssyc

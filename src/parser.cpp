#include "parser.h"

extern FILE *yyin, *yyout;
extern int   yyparse();

namespace ssyc {

bool Parser::setSource(std::string_view sourcePath) noexcept {
    namespace fs      = std::filesystem;
    const auto path   = fs::path(sourcePath).generic_string();
    const auto handle = fopen(path.c_str(), "r");
    if (handle != nullptr) {
        if (optInStream.has_value()) { fclose(optInStream.value()); }
        optInStream.emplace(handle);
        return true;
    }
    return false;
}

bool Parser::execute() {
    yyin = optInStream.value_or(stdin);

    const auto resp = yyparse();

    yyin = nullptr;
    return resp;
}

} // namespace ssyc

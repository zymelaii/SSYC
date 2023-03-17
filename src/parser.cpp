#include "parser.h"

extern FILE *yyin, *yyout;
extern int   yyparse(ssyc::ParserContext &context);

namespace ssyc {

Parser::Parser()
    : optInStream{}
    , ptrContext{nullptr} {}

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

    ptrContext.reset(new ParserContext);
    const auto resp = yyparse(*ptrContext.get());

    yyin = nullptr;
    return resp;
}

ParserContext *Parser::context() {
    //! FIXME: 不安全的指针
    return ptrContext.get();
}

} // namespace ssyc

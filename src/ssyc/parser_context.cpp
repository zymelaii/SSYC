#include "parser_context.h"

namespace ssyc {

ParserContext::ParserContext()
    : program{new ast::Program} {}

ParserContext::~ParserContext() {
    for (const auto &e : units) {
        delete e;
    }
    program = nullptr;
}

} // namespace ssyc

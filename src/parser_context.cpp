#include "parser_context.h"

namespace ssyc {

ParserContext::ParserContext()
    : program{new ast::Program} {}

ParserContext::~ParserContext() {}

} // namespace ssyc

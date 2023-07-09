#include "driver.h"
#include "../visitor/ASTDumpVisitor.h"
#include "../visitor/ASTToIRTranslator.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

namespace slime {

Driver::Driver()
    : ready_{false}
    , parser_{}
    , flags_{} {}

Driver* Driver::create() {
    return new Driver;
}

Driver* Driver::withSourceFile(const char* path) {
    std::ifstream ifs(path);
    if (ifs.is_open()) { resetInput(ifs); }
    return this;
}

Driver* Driver::withStdin() {
    std::string       input;
    std::stringstream ss;
    std::getline(std::cin, input, '\0');
    ss << input;
    resetInput(ss);
    return this;
}

Driver* Driver::withFlags(const Flags& flags) {
    flags_ = flags;
    return this;
}

bool Driver::isReady() const {
    return ready_;
}

void Driver::execute() {
    assert(isReady() && "driver is not ready");

    TranslationUnit* unit = nullptr;
    bool             done = parser_.lexer().this_token().isEOF();

    if (!done && flags_.LexOnly) {
        char buf[256]{};
        auto lexer = parser_.unbindLexer();
        do {
            const char* tok = tok2str(lexer->this_token(), buf);
            puts(pretty_tok2str(lexer->this_token(), buf));
        } while (!lexer->exact_next().isEOF());
        done = true;
    }

    if (!done) {
        unit = parser_.parse();
        assert(unit != nullptr && "invalid parse result");
    }

    if (!done && flags_.DumpAST) {
        auto visitor = visitor::ASTDumpVisitor::createWithOstream(&std::cout);
        visitor->visit(unit);
    }

    if (!done && flags_.EmitIR) {
        auto module = visitor::ASTToIRTranslator::translate("default", unit);
    }

    ready_ = false;
}

} // namespace slime

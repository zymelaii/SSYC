#include "driver.h"
#include "../visitor/ASTDumpVisitor.h"

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

    auto&            ls     = parser_.ls;
    TranslationUnit* module = nullptr;
    bool             done   = ls.token.id == TOKEN::TK_EOF;

    if (!done && flags_.LexOnly) {
        char buf[256]{};
        while (ls.token.id != TOKEN::TK_EOF) {
            const char* tok = tok2str(ls.token.id, buf);
            puts(pretty_tok2str(ls.token, buf));
            ls.next();
        }
        done = true;
    }

    if (!done) {
        module = parser_.parse();
        assert(module != nullptr && "invalid parse result");
    }

    if (!done && flags_.DumpAST) {
        auto visitor = visitor::ASTDumpVisitor::createWithOstream(&std::cout);
        visitor->visit(module);
    }

    ready_ = false;
}

} // namespace slime

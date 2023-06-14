#include "driver.h"

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
    auto& ls = parser_.ls;

    if (flags_.LexOnly) {
        char buf[256]{};
        while (ls.token.id != TOKEN::TK_EOF) {
            const char* tok = tok2str(ls.token.id, buf);
            puts(pretty_tok2str(ls.token, buf));
            ls.next();
        }
    } else {
        ASTNode* root;
        while (ls.token.id != TOKEN::TK_EOF) {
            root = parser_.global_parse();
            //!TODO: display info of uninitialised symbol
            if(!root) printf("Uninitialised global symbol defined.\n");
            else{
                printf("Global Symbol defined: \n");
                if(root->op == A_FUNCTION) parser_.displaySymInfo(root->val.symindex, NULL);
                else parser_.displaySymInfo(root->left->val.symindex, NULL);
            }
            parser_.traverseAST(root);
            printf("\n");
        }
    }

    ready_ = false;
}

} // namespace slime

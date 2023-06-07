#include "parser.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    Parser parser;
    bool   has_input = false;
    if (argc > 1) {
        std::ifstream ifs(argv[1]);
        if (ifs.is_open()) {
            parser.ls.reset(ifs);
            has_input = true;
        }
    }
    if (!has_input) {
        std::string       input;
        std::stringstream ss;
        std::getline(std::cin, input, '\0');
        ss << input;
        parser.ls.reset(ss);
    }

    // char buf[256]{};
    // while (parser.ls.token.id != TOKEN::TK_EOF) {
    //     parser.ls.next();
    //     const char* tok = tok2str(parser.ls.token.id, buf);
    //     puts(pretty_tok2str(parser.ls.token, buf));
    // }

    parser.next();
    while (parser.ls.token.id != TOKEN::TK_EOF) { parser.func(); }

    return 0;
}

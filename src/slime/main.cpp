#include "lex.h"
#include <sstream>
#include <iostream>
#include <regex>
#include <string>
#include <fstream>

int main(int argc, char* argv[]) {
    LexState ls;
    bool     has_input = false;
    if (argc > 2) {
        std::ifstream ifs(argv[1]);
        if (ifs.is_open()) {
            ls.reset(ifs);
            has_input = true;
        }
    }
    if (!has_input) {
        std::string       input;
        std::stringstream ss;
        std::getline(std::cin, input, '\0');
        ss << input;
        ls.reset(ss);
    }

    char buf[256]{};
    while (ls.token.id != TOKEN::TK_EOF) {
        ls.next();
        const char* tok = tok2str(ls.token.id, buf);
        puts(pretty_tok2str(ls.token, buf));
    }

    return 0;
}

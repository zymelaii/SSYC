#include "lex.h"
#include <sstream>
#include <iostream>
#include <regex>
#include <string>
#include <fstream>

extern TOKEN read_number(LexState& ls);

int main(int argc, char* argv[]) {
    std::ifstream ifs(
        argc > 1 ? argv[1]
                 : R"(E:\Storage\FreeSpace\workspace\SSYC\src\slime\foobar.c)");

    LexState ls;
    ls.reset(ifs);

    while (ls.token.id != TOKEN::TK_EOF) {
        ls.next();
        char buf[32]{};
        tok2str(ls.token.id, buf, 32);
        printf(
            "<%zu:%zu> %s: %s\n",
            ls.line,
            ls.column,
            buf,
            ls.token.detail.data());
    }

    return 0;
}

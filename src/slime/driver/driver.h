#pragma once

#include "optman.h"

#include <slime/parse/parser.h>
#include <istream>
#include <string>

namespace slime {

class Driver {
public:
    static Driver* create();
    void           execute(int argc, char** argv);

protected:
    Driver();

    template <
        typename T,
        typename = std::enable_if_t<std::is_base_of_v<std::istream, T>>>
    void resetInput(T& is) {
        if (!is.eof()) { parser_.reset(is, currentSource_.c_str()); }
    }

private:
    OptManager  optman_;
    Parser      parser_;
    std::string currentSource_;
};

} // namespace slime

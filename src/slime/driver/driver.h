#pragma once

#include <slime/parse/parser.h>
#include <istream>
#include <string>

namespace slime {

class Driver {
public:
    struct Flags {
        bool LexOnly      = false;
        bool DumpAST      = false;
        bool EmitIR       = false;
        bool DumpAssembly = false;
    };

    static Driver* create();

    Driver* withStdin();
    Driver* withSourceFile(const char* path);
    Driver* withFlags(const Flags& flags);

    bool isReady() const;
    void execute();

protected:
    Driver();

    template <
        typename T,
        typename = std::enable_if_t<std::is_base_of_v<std::istream, T>>>
    void resetInput(T& is) {
        if (!is.eof()) {
            parser_.reset(is, currentSource_.c_str());
            ready_ = true;
        }
    }

private:
    bool        ready_;
    Parser      parser_;
    Flags       flags_;
    std::string currentSource_;
};

} // namespace slime

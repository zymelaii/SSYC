#include <slime/driver/driver.h>
#include <iostream>
#include <getopt.h>
#include <string.h>
#include <memory>

using slime::Driver;

int main(int argc, char* argv[]) {
    std::unique_ptr<Driver> driver{Driver::create()};
    Driver::Flags           flags{};

    static option opts[]{
        {"lex-only", no_argument, 0, 0},
        {"dump-ast", no_argument, 0, 0},
        {"emit-ir",  no_argument, 0, 0},
    };

    int optval = -1;
    int optidx = -1;

    while ((optval = getopt_long(argc, argv, "", opts, &optidx)) != -1) {
        if (optval == 0) {
            if (strcmp(opts[optidx].name, "lex-only") == 0) {
                flags.LexOnly = true;
                if (flags.DumpAST) { flags.DumpAST = false; }
            } else if (strcmp(opts[optidx].name, "dump-ast") == 0) {
                if (!flags.LexOnly) { flags.DumpAST = true; }
            } else if (strcmp(opts[optidx].name, "emit-ir") == 0) {
                flags.EmitIR = true;
            }
        }
    }

    for (int i = optind; i < argc; ++i) {
        if (driver->withSourceFile(argv[i])->isReady()) { break; }
    }

    if (!driver->isReady()) { driver->withStdin(); }

    driver->withFlags(flags)->execute();

    return 0;
}

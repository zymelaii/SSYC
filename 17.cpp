#ifndef SLIMEC_VERSION
#define SLIMEC_VERSION "alpha"
#endif

#ifndef SLIMEC_HOMEPAGEURL
#define SLIMEC_HOMEPAGEURL "https://github.com/zymelaii/SSYC"
#endif

#ifndef SLIMEC_STDLIB
#define SLIMEC_STDLIB "unknown"
#endif

#include "18.h"
#include "20.h"

#include "58.h"
#include "92.h"
#include "96.h"
#include "98.h"
#include "14.h"
#include "68.h"
#include "80.h"
#include "76.h"
#include <filesystem>
#include <iostream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdlib.h>

namespace fs = std::filesystem;

namespace slime {

Driver::Driver()
    : parser_{} {
    optman_
        .withOption('h', "help", false)     //<! -h, --help
        .withOption('v', "version", false)  //<! -v, --version
        .withExtOption('f')                 //<! -f{args...}
        .withExtOption('O')                 //<! -O{args...}
        .withOption('o', "output", "a.out") //<! -o, --output <path-to-output>
        .withOption(0, "lex-only", false)   //<! --lex-only
        .withOption(0, "emit-ir", false)    //<! --emit-ir
        .withOption(0, "dump-ast", false)   //<! --dump-ast
        .withOption('E', nullptr, false)    //<! -E
        .withOption('S', nullptr, false)    //<! -S
        .withOption('-', nullptr, false)    //<! --
        .withOption('p', "pipe", false)     //<! -p, --pipe
        ;
}

Driver* Driver::create() {
    return new Driver;
}

void Driver::execute(int argc, char** argv) {
    optman_.parse(argc, argv);

    if (auto PrintHelp = optman_.valueOfSwitch('h')) {
        puts(
            "ARM-v7a assembly compiler for SysY wriiten in C++ 17"
            "\n\n"
            "Usage: slimec [options] file..."
            "\n\n"
            "Options:"
            "\n  -h, --help           Print help information"
            "\n  -v, --version        Print version information"
            "\n  --                   Read from stdin"
            "\n  -E                   Only run the preprocessor"
            "\n  --lex-only           Only run the lexer"
            "\n  -S                   Only run preprocess and compilation steps"
            "\n  --dump-ast           Use the AST representation for output"
            "\n  --emit-ir            Use the MIR representation for assembler"
            "\n  -o, --output <file>  Write output to <file>"
            "\n  -p, --pipe           Always output to stdout"
            "\n  -O<opt-level>        Specify optimize level [UNCOMPLETE]"
            "\n  -f<flag>             Provide extra flag [UNCOMPLETE]"
            "\n ");
        return;
    }

    if (auto PrintVersion = optman_.valueOfSwitch('v')) {
        puts("slimec version " SLIMEC_VERSION " (" SLIMEC_HOMEPAGEURL ")"
             "\n"
             "Built with standard library: " SLIMEC_STDLIB);
        return;
    }

    std::istream* stream             = nullptr;
    bool          IsCurrentFromStdin = false;

    bool ReadFromStdin = optman_.valueOfSwitch('-');
    if (ReadFromStdin) {
        std::string input;
        auto        ss = new std::stringstream;
        std::getline(std::cin, input, '\0');
        *ss << input;
        currentSource_.clear();
        stream             = ss;
        IsCurrentFromStdin = true;
    }

    std::vector<std::string> sourceFiles;
    int                      nextFileIndex = 0;
    if (!ReadFromStdin) {
        for (auto file : optman_.rest()) {
            if (!fs::exists(file) || !fs::is_regular_file(file)) {
                fprintf(
                    stderr,
                    "warning: file does not exist or is invalid: \"%s\"\n",
                    file.data());
                continue;
            }
            sourceFiles.push_back(std::string{file});
        }
        assert(sourceFiles.size() > 0);
        auto path = sourceFiles[nextFileIndex++];
        auto ifs  = new std::ifstream(path);
        assert(ifs->is_open());
        currentSource_ = path;
        stream         = ifs;
    }

    if (!stream) {
        fputs("error: no input files", stderr);
        exit(-1);
    }

    bool PreprocessOnly = optman_.valueOfSwitch('E');
    bool LexOnly        = optman_.valueOfSwitch("lex-only");
    bool DumpAST        = optman_.valueOfSwitch("dump-ast");
    bool EmitIR         = optman_.valueOfSwitch("emit-ir");
    bool DumpAssembly   = optman_.valueOfSwitch('S');
    bool ForcePipe      = optman_.valueOfSwitch('p');
    bool OutputToStdout = !optman_.provided('o') || ForcePipe;

    if (!DumpAssembly) {
        fputs("error: objects generation is not supported", stderr);
        exit(-1);
    }

    auto OutputFile = std::string{optman_.valueOfArgument('o')};
    if (!OutputToStdout) {
        if (sourceFiles.size() > 1) {
            fputs(
                "error: cannot specify -o when generating multiple output "
                "files",
                stderr);
            exit(-1);
        }
        std::ofstream ostream(OutputFile);
        if (!ostream.is_open()) {
            fprintf(
                stderr,
                "error: unable to open output file: \"%s\"\n",
                OutputFile.data());
            exit(-1);
        }
    }

    while (true) {
        TranslationUnit* unit    = nullptr;
        ir::Module*      module  = nullptr;
        std::ostream*    ostream = nullptr;
        if (OutputToStdout) {
            ostream = &std::cout;
        } else {
            ostream = new std::ofstream(OutputFile);
        }

        if (PreprocessOnly) {
            InputStreamTransformer transformer;
            transformer.reset(stream, currentSource_);
            while (true) {
                char ch = transformer.get();
                if (ch == static_cast<char>(EOF)) { break; }
                ostream->put(ch);
            }
            goto nextStep;
        }

        if (IsCurrentFromStdin) {
            resetInput(static_cast<std::stringstream&>(*stream));
        } else {
            resetInput(static_cast<std::ifstream&>(*stream));
        }

        if (LexOnly) {
            char buf[256]{};
            auto lexer = parser_.unbindLexer();
            if (!lexer->this_token().isEOF()) {
                do {
                    const char* tok = tok2str(lexer->this_token(), buf);
                    *ostream << pretty_tok2str(lexer->this_token(), buf)
                             << "\n";
                } while (!lexer->exact_next().isEOF());
            }
            goto nextStep;
        }

        unit = parser_.parse();
        assert(unit != nullptr);

        if (DumpAST) {
            auto dumper = visitor::ASTDumpVisitor::createWithOstream(ostream);
            dumper->visit(unit);
            goto nextStep;
        }

        module = visitor::ASTToIRTranslator::translate(
            currentSource_.empty() ? "<stdin>" : currentSource_.c_str(), unit);
        pass::ControlFlowSimplificationPass{}.run(module);
        pass::ResortPass{}.run(module);
        pass::MemoryToRegisterPass{}.run(module);

        if (EmitIR) {
            auto visitor = visitor::IRDumpVisitor::createWithOstream(ostream);
            visitor->dump(module);
            goto nextStep;
        }

        if (DumpAssembly) {
            auto armv7aGen = backend::Generator::generate();
            *ostream << armv7aGen->genCode(module);
            goto nextStep;
        }

        unreachable();

nextStep:
        if (!OutputToStdout) { delete ostream; }
        delete stream;
        stream = nullptr;
        if (ReadFromStdin || nextFileIndex == sourceFiles.size()) { break; }
        auto path = sourceFiles[nextFileIndex++];
        auto ifs  = new std::ifstream(path);
        assert(ifs->is_open());
        currentSource_     = path;
        stream             = ifs;
        IsCurrentFromStdin = false;
    }
}

} // namespace slime

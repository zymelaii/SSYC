#include <glog/logging.h>
#include <gflags/gflags.h>
#include <filesystem>
#include <iostream>

#include "parser.h"
#include "utils.h"

using ssyc::Parser;

DEFINE_string(input, "\xff", "input files");

DEFINE_validator(input, [](const char* flag, const std::string& value) -> bool {
    namespace fs = std::filesystem;

    if (value.empty()) { return false; }

    if (value[0] == '\xff') { return true; }

    if (!fs::exists(value)) {
        LOG(ERROR) << "SSYC: error: no such file: '" << value << "'";
        return false;
    }

    return true;
});

int main(int argc, char* argv[]) {
    FLAGS_colorlogtostderr = true;
    FLAGS_logtostderr      = true;
    google::SetStderrLogging(google::GLOG_INFO);
    google::InitGoogleLogging(argv[0]);

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    Parser parser{};

    gflags::CommandLineFlagInfo info{};
    if (gflags::GetCommandLineFlagInfo("input", &info); !info.is_default) {
        const auto resp = parser.setSource(FLAGS_input);
        LOG_IF(WARNING, !resp) << "unexpected source redirection failure";
    }

    parser.execute();

    auto context = parser.context();
    auto node    = context->program;
    auto visit   = [](auto node) {
        auto self = [](auto&& self, auto node) {
            if (!node) return;
            if (!node->left && !node->right && !node->FLAGS_Nested) {
                std::cout << "{token: " << tokenEnumToString(node->token);
                if (!std::holds_alternative<std::monostate>(node->value)) {
                    if (std::holds_alternative<std::string>(node->value)) {
                        std::cout << " `" << std::get<std::string>(node->value)
                                  << "'";
                    } else if (std::holds_alternative<int>(node->value)) {
                        std::cout << " `" << std::get<int>(node->value) << "'";
                    } else if (std::holds_alternative<float>(node->value)) {
                        std::cout << " `" << std::get<float>(node->value)
                                  << "'";
                    }
                }
                std::cout << "} ";
            } else {
                std::cout << "{syntax: " << syntaxEnumToString(node->syntax);

                if (!std::holds_alternative<std::monostate>(node->value)) {
                    if (std::holds_alternative<std::string>(node->value)) {
                        std::cout << " `" << std::get<std::string>(node->value)
                                  << "'";
                    } else if (std::holds_alternative<int>(node->value)) {
                        std::cout << " `" << std::get<int>(node->value) << "'";
                    } else if (std::holds_alternative<float>(node->value)) {
                        std::cout << " `" << std::get<float>(node->value)
                                  << "'";
                    }
                }
                std::cout << "} ";
                self(self, node->left);
                self(self, node->right);
            }
        };
        return self(self, node);
    };

    visit(node);

    return 0;
}

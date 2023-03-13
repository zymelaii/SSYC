#include <glog/logging.h>
#include <gflags/gflags.h>
#include <filesystem>

#include "parser.h"

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
        LOG_IF(WARNING, resp) << "unexpected source redirection failure";
    }

    parser.execute();

    return 0;
}
#pragma once

#include <map>
#include <string>
#include <string_view>

namespace slime {

class OptManager {
public:
    OptManager& withOption(char opt, const char* lopt, bool defaultValue);
    OptManager& withOption(
        char opt, const char* lopt, const char* defaultValue);
    OptManager& withExtOption(char prefix);

    bool valueOfSwitch(char opt) const;
    bool valueOfSwitch(const char* lopt) const;

    std::string_view valueOfArgument(char opt) const;
    std::string_view valueOfArgument(const char* lopt) const;

    std::vector<std::string_view> extOptionValues(char opt) const;
    std::vector<std::string_view> rest() const;

    bool provided(char opt) const;
    bool provided(const char* lopt) const;

    void parse(int argc, char** argv);
    void reset();

private:
    using OptionID = int;

    static constexpr OptionID nopt = 0;

    std::map<std::string_view, OptionID> loptMap_;
    std::array<OptionID, 256>            optMap_;

    std::map<OptionID, bool>        defaultSwitchOptions_;
    std::map<OptionID, std::string> defaultValueOptions_;

    std::map<OptionID, bool>        switchOptions_;
    std::map<OptionID, std::string> valueOptions_;

    std::map<char, std::vector<std::string>> extendedOptions_;

    std::vector<std::string> rest_;
};

} // namespace slime
#include "optman.h"

#include <assert.h>

namespace slime {

OptManager& OptManager::withOption(
    char opt, const char* lopt, bool defaultValue) {
    auto nextId =
        defaultSwitchOptions_.size() + defaultValueOptions_.size() + 1;
    assert(nextId != nopt);
    if (optMap_[opt] != nopt) {
        defaultSwitchOptions_.erase(optMap_[opt]);
        optMap_[opt] = nopt;
    }
    if (lopt && loptMap_.count(lopt)) {
        if (defaultSwitchOptions_.count(loptMap_[lopt])) {
            defaultSwitchOptions_.erase(loptMap_[lopt]);
        }
        loptMap_.erase(lopt);
    }
    if (opt != 0) { optMap_[opt] = nextId; }
    if (lopt != nullptr) { loptMap_[lopt] = nextId; }
    defaultSwitchOptions_[nextId] = defaultValue;
    return *this;
}

OptManager& OptManager::withOption(
    char opt, const char* lopt, const char* defaultValue) {
    auto nextId =
        defaultSwitchOptions_.size() + defaultValueOptions_.size() + 1;
    assert(nextId != nopt);
    if (optMap_[opt] != nopt) {
        defaultValueOptions_.erase(optMap_[opt]);
        optMap_[opt] = nopt;
    }
    if (lopt && loptMap_.count(lopt)) {
        if (defaultValueOptions_.count(loptMap_[lopt])) {
            defaultValueOptions_.erase(loptMap_[lopt]);
        }
        loptMap_.erase(lopt);
    }
    if (opt != 0) { optMap_[opt] = nextId; }
    if (lopt != nullptr) { loptMap_[lopt] = nextId; }
    if (!defaultValue) { defaultValue = ""; }
    defaultValueOptions_[nextId] = defaultValue ? defaultValue : "";
    return *this;
}

OptManager& OptManager::withExtOption(char prefix) {
    if (!extendedOptions_.count(prefix)) { extendedOptions_[prefix] = {}; }
    return *this;
}

bool OptManager::valueOfSwitch(char opt) const {
    assert(optMap_[opt] != nopt);
    auto id = optMap_[opt];
    return switchOptions_.count(id) ? switchOptions_.at(id)
                                    : defaultSwitchOptions_.at(id);
}

bool OptManager::valueOfSwitch(const char* lopt) const {
    assert(lopt && loptMap_.count(lopt));
    auto id = loptMap_.at(lopt);
    return switchOptions_.count(id) ? switchOptions_.at(id)
                                    : defaultSwitchOptions_.at(id);
}

std::string_view OptManager::valueOfArgument(char opt) const {
    assert(optMap_[opt] != nopt);
    auto id = optMap_[opt];
    return valueOptions_.count(id) ? valueOptions_.at(id)
                                   : defaultValueOptions_.at(id);
}

std::string_view OptManager::valueOfArgument(const char* lopt) const {
    assert(lopt && loptMap_.count(lopt));
    auto id = loptMap_.at(lopt);
    return valueOptions_.count(id) ? valueOptions_.at(id)
                                   : defaultValueOptions_.at(id);
}

std::vector<std::string_view> OptManager::extOptionValues(char opt) const {
    assert(extendedOptions_.count(opt));
    const auto& opts = extendedOptions_.at(opt);

    std::vector<std::string_view> resp;
    resp.reserve(opts.size());
    for (const auto& e : opts) { resp.push_back(e); }
    return resp;
}

std::vector<std::string_view> OptManager::rest() const {
    std::vector<std::string_view> resp;
    resp.reserve(rest_.size());
    for (const auto& e : rest_) { resp.push_back(e); }
    return resp;
}

bool OptManager::provided(char opt) const {
    return optMap_[opt] != nopt;
}

bool OptManager::provided(const char* lopt) const {
    return lopt && loptMap_.count(lopt);
}

void OptManager::parse(int argc, char** argv) {
    reset();
    for (int i = 0; i < argc; ++i) {
        char* argument = argv[i];
        auto  id       = nopt;
        if (argument[0] != '-' || argument[1] == '\0') {
            rest_.push_back(argument);
            continue;
        }
        if (argument[1] == '-' && argument[2] == '\0') {
            char opt = '-';
            if (optMap_[opt] == nopt) {
                rest_.push_back(argument);
                continue;
            }
            id = optMap_[opt];
        } else if (argument[1] == '-') {
            const char* lopt = &argument[2];
            if (!loptMap_.count(lopt)) {
                rest_.push_back(argument);
                continue;
            }
            id = loptMap_[lopt];
        } else if (argument[2] == '\0') {
            char opt = argument[1];
            if (optMap_[opt] == nopt) {
                rest_.push_back(argument);
                continue;
            }
            id = optMap_[opt];
        }
        if (id != nopt) {
            if (defaultSwitchOptions_.count(id)) {
                switchOptions_[id] = !defaultSwitchOptions_[id];
            } else {
                assert(i + 1 < argc);
                auto value        = argv[++i];
                valueOptions_[id] = value;
            }
            continue;
        }
        char prefix = argument[1];
        if (!extendedOptions_.count(prefix)) {
            rest_.push_back(argument);
        } else {
            extendedOptions_[prefix].push_back(&argument[2]);
        }
    }
}

void OptManager::reset() {
    switchOptions_.clear();
    valueOptions_.clear();
    for (auto& [_, v] : extendedOptions_) { v.clear(); }
    rest_.clear();
}

} // namespace slime

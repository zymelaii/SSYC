/*!
 * \brief IR Module
 *
 * IR Module 定义了一个完整的程序模块
 */

#pragma once

#include "context.h"

#include <string>
#include <stdexcept>
#include <ostream>
#include <optional>

namespace ssyc::ir {

class Module {
public:
    inline Module(std::string_view id, Context* context)
        : id_{id}
        , context_{context} {
        if (id_.empty()) {
            throw std::runtime_error("IR Module ID cannot be empty");
        }
    }

    ~Module();

    inline std::string_view id() const {
        return id_;
    }

    inline Context& context() const {
        return *context_;
    }

    void dump(std::optional<std::reference_wrapper<std::ostream>> optOs = {});

private:
    std::string    id_;
    Context* const context_;
};

} // namespace ssyc::ir

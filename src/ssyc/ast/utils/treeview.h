#pragma once

#include "../ast.h"
#include "translate.h"

#include <ostream>
#include <iomanip>

namespace ssyc::ast::utils {

std::ostream &operator<<(std::ostream &os, const ast_node auto *root) {
    TreeWalkState state(root);

    auto node = state.next();
    while (node != nullptr) {
        os << std::setw(state.depth()) << std::setfill('|') << ""
           << getNodeBrief(node) << " 0x"
           << reinterpret_cast<const void *>(node) << std::endl;
        node = state.next();
    }

    return os;
}

}; // namespace ssyc::ast::utils

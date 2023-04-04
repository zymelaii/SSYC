#pragma once

#include "../ast.h"
#include "translate.h"

#include <iostream>

namespace ssyc::ast::utils {

void treeview(const ast_node auto *root) {
    TreeWalkState state(root);

    //auto it = std::begin(state.flattenTreeList);

    //*state.cursor++ = root;
    //state.walkState.push(1);

    //while (!state.walkState.empty()) {
    //    auto e = *it++;
    //    --state.walkState.top();
    //    std::cerr << state.walkState.size() - 1 << "] " << getNodeBrief(e)
    //              << std::endl;

    //    e->flattenInsert(state);

    //    if (state.walkState.top() == 0) { state.walkState.pop(); }
    //}
}

}; // namespace ssyc::ast::utils

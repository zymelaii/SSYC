#include "parser_context.h"

#include <assert.h>
#include <queue>

namespace ssyc {

ParserContext::ParserContext()
    : freelist{}
    , program{nullptr}
    , cursor{nullptr} {}

ParserContext::~ParserContext() {
    for (auto e : freelist) { delete e; }
    freelist.clear();

    if (program == nullptr) {
        assert(cursor == nullptr);
        return;
    }

    delete program;
    program = nullptr;
    cursor  = nullptr;
}

AstNode *ParserContext::require() {
    if (freelist.empty()) return new AstNode;
    auto it   = freelist.begin();
    auto node = *it;
    freelist.erase(it);
    return node;
}

void ParserContext::deleteLater(AstNode *node) {
    if (node == nullptr) return;
    if (node->parent != nullptr) {
        if (node->parent->left == node) { node->parent->left = nullptr; }
        if (node->parent->right == node) { node->parent->right = nullptr; }
    }
    deleteLater(node->left);
    deleteLater(node->right);
    new (node) AstNode;
    freelist.insert(node);
}

} // namespace ssyc

#include "ast.h"

namespace ssyc {

AstNode::AstNode()
    : flags{}
    , scope{0}
    , parent{nullptr}
    , left{nullptr}
    , right{nullptr} {}

AstNode::~AstNode() {
    if (left != nullptr) {
        delete left;
        left = nullptr;
    }

    if (right != nullptr) {
        delete right;
        right = nullptr;
    }
}

} // namespace ssyc

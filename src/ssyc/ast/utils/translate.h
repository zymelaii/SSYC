#include "../ast.h"

#include <string>
#include <string_view>
#include <sstream>
#include <array>

namespace ssyc::ast::utils {

using ssyc::ast::NodeBrief;
using std::operator""sv;

namespace details {

template <typename T, typename First, typename... Left>
consteval bool contains() {
    constexpr auto found = std::is_same_v<T, First>;
    if constexpr (found) {
        return true;
    } else if constexpr (sizeof...(Left) == 0) {
        return false;
    } else {
        return contains<T, Left...>();
    }
}

} // namespace details

inline void acquireBriefInfo(NodeBrief &brief, const ast_node auto *e) {
    using node_type = std::remove_cvref_t<decltype(*e)>;

    constexpr auto isBaseType = details::contains<
        node_type,
        AbstractAstNode,
        Type,
        Decl,
        Expr,
        Stmt,
        Literal,
        ArrayType>();

    //! 剔除不必要的虚函数开销
    if constexpr (isBaseType) {
        e->acquire(brief);
    } else {
        e->node_type::acquire(brief);
    }
}

std::string getNodeBrief(const ast_node auto *e) {
    NodeBrief brief;
    acquireBriefInfo(brief, e);

    std::stringstream ss;

    constexpr auto baseTypeNum = static_cast<size_t>(NodeBaseType::Unknown) + 1;
    constexpr auto typeTable   = std::array<std::string_view, baseTypeNum>{
        "Type"sv, "Decl"sv, "Expr"sv, "Stmt"sv, "?"sv};

    ss << "<"sv << typeTable[static_cast<size_t>(brief.type)] << "> "sv;
    ss << "<"sv << (brief.name.empty() ? "?"sv : brief.name) << "> "sv;
    ss << "<"sv << brief.hint << ">"sv;

    return ss.str();
}

}; // namespace ssyc::ast::utils

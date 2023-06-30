#include <gtest/gtest.h>
#include <slime/utils/list.h>
#include <sstream>

using slime::utils::ListTrait;

TEST(ListTraitOperations, ForwardTranverse) {
    ListTrait<int> l;
    l.insertToTail(1);
    l.insertToTail(2);
    l.insertToTail(3);
    std::stringstream ss;
    for (auto e : l) { ss << e; }
    ASSERT_TRUE(ss.str() == "123");
}

TEST(ListTraitOperations, BackwardTranverse) {
    ListTrait<int> l;
    l.insertToTail(1);
    l.insertToTail(2);
    l.insertToTail(3);
    std::stringstream ss;
    for (auto it = l.rbegin(); it != l.rend(); ++it) { ss << *it; }
    ASSERT_TRUE(ss.str() == "321");
}

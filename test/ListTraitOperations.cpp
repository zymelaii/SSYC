#include <gtest/gtest.h>
#include <slime/experimental/utils/LinkedList.h>
#include <sstream>
#include <string>

using slime::LinkedList;

std::string to_string(const LinkedList<int> &l) {
    std::stringstream ss;
    if (l.size() == 1) {
        return std::to_string(l.head()->value());
    } else if (l.size() > 1) {
        auto it = l.begin();
        ss << *it++;
        while (it != l.end()) { ss << " " << *it++; }
    }
    return ss.str();
}

TEST(ListTraitOperations, TrivialCtor) {
    struct foo {
        int v;
    };

    LinkedList<int>::node_type p1{};
    LinkedList<foo>::node_type p2{};
}

TEST(ListTraitOperations, NonTrivialCtor) {
    struct bar {
        bar() = default;

        int v;
    };

    LinkedList<bar>::node_type p3{};
}

TEST(ListTraitOperations, CreateForward) {
    struct Foo {
        Foo()
            : value{-1} {}

        Foo(int e)
            : value{e} {}

        int value;
    };

    auto p1 = LinkedList<int>::create();
    ASSERT_EQ(p1->value(), 0);

    auto p2 = LinkedList<int>::create(3);
    ASSERT_EQ(p2->value(), 3);

    auto p3 = LinkedList<Foo>::create(2333);
    ASSERT_EQ(p3->value().value, 2333);

    auto p4 = LinkedList<Foo>::create();
    ASSERT_EQ(p4->value().value, -1);
}

TEST(ListTraitOperations, InsertToHead) {
    LinkedList<int> l{};

    LinkedList<int>::node_type *p = nullptr;

    ASSERT_EQ(to_string(l), "");

    l.insertToHead(3);
    ASSERT_EQ(to_string(l), "3");

    l.insertToHead(2);
    ASSERT_EQ(to_string(l), "2 3");

    p = l.create(1);
    l.insertToHead(p);
    ASSERT_EQ(to_string(l), "1 2 3");
}

TEST(ListTraitOperations, InsertToTail) {
    LinkedList<int> l{};

    LinkedList<int>::node_type *p = nullptr;

    ASSERT_EQ(to_string(l), "");

    l.insertToTail(1);
    ASSERT_EQ(to_string(l), "1");

    l.insertToTail(2);
    ASSERT_EQ(to_string(l), "1 2");

    p = l.create(3);
    l.insertToTail(p);
    ASSERT_EQ(to_string(l), "1 2 3");
}

TEST(ListTraitOperations, MoveToPrev) {
    LinkedList<int> l{};

    LinkedList<int>::node_type *p = nullptr;

    l.insertToTail(1);
    ASSERT_EQ(to_string(l), "1");

    auto mid = l.insertToTail(2);
    l.insertToTail(3);
    ASSERT_EQ(to_string(l), "1 2 3");

    mid->moveToPrev();
    ASSERT_EQ(to_string(l), "2 1 3");

    mid->moveToPrev();
    ASSERT_EQ(to_string(l), "2 1 3");
}

TEST(ListTraitOperations, MoveToNext) {
    LinkedList<int> l{};

    LinkedList<int>::node_type *p = nullptr;

    auto first = l.insertToTail(1);
    ASSERT_EQ(to_string(l), "1");

    l.insertToTail(2);
    l.insertToTail(3);
    ASSERT_EQ(to_string(l), "1 2 3");

    first->moveToNext();
    ASSERT_EQ(to_string(l), "2 1 3");

    first->moveToNext();
    ASSERT_EQ(to_string(l), "2 3 1");

    first->moveToNext();
    ASSERT_EQ(to_string(l), "2 3 1");
}

TEST(ListTraitOperations, MoveBefore) {
    LinkedList<int> l{};

    auto p1 = l.insertToTail(1);
    auto p2 = l.insertToTail(2);
    auto p3 = l.insertToTail(3);
    auto p4 = l.insertToTail(4);
    auto p5 = l.insertToTail(5);

    ASSERT_EQ(to_string(l), "1 2 3 4 5");

    p1->moveBefore(p5);
    ASSERT_EQ(to_string(l), "2 3 4 1 5");

    p2->moveBefore(p1);
    ASSERT_EQ(to_string(l), "3 4 2 1 5");

    p1->moveBefore(p1);
    ASSERT_EQ(to_string(l), "3 4 2 1 5");

    p2->moveBefore(p1);
    ASSERT_EQ(to_string(l), "3 4 2 1 5");

    p5->moveBefore(p1);
    ASSERT_EQ(to_string(l), "3 4 2 5 1");

    p1->moveBefore(p4);
    ASSERT_EQ(to_string(l), "3 1 4 2 5");

    p4->moveBefore(p3);
    ASSERT_EQ(to_string(l), "4 3 1 2 5");

    p4->moveBefore(p1);
    ASSERT_EQ(to_string(l), "3 4 1 2 5");
}

TEST(ListTraitOperations, MoveAfter) {
    LinkedList<int> l{};

    auto p1 = l.insertToTail(1);
    auto p2 = l.insertToTail(2);
    auto p3 = l.insertToTail(3);
    auto p4 = l.insertToTail(4);
    auto p5 = l.insertToTail(5);

    ASSERT_EQ(to_string(l), "1 2 3 4 5");

    p1->moveAfter(p5);
    ASSERT_EQ(to_string(l), "2 3 4 5 1");

    p2->moveAfter(p1);
    ASSERT_EQ(to_string(l), "3 4 5 1 2");

    p1->moveAfter(p1);
    ASSERT_EQ(to_string(l), "3 4 5 1 2");

    p2->moveAfter(p1);
    ASSERT_EQ(to_string(l), "3 4 5 1 2");

    p1->moveAfter(p5);
    ASSERT_EQ(to_string(l), "3 4 5 1 2");

    p2->moveAfter(p5);
    ASSERT_EQ(to_string(l), "3 4 5 2 1");

    p3->moveAfter(p5);
    ASSERT_EQ(to_string(l), "4 5 3 2 1");

    p2->moveAfter(p1);
    ASSERT_EQ(to_string(l), "4 5 3 1 2");

    p3->moveAfter(p2);
    ASSERT_EQ(to_string(l), "4 5 1 2 3");
}

TEST(ListTraitOperations, Swap) {
    LinkedList<int> l{};

    auto p1 = l.insertToTail(1);
    auto p2 = l.insertToTail(2);
    auto p3 = l.insertToTail(3);
    auto p4 = l.insertToTail(4);
    auto p5 = l.insertToTail(5);

    ASSERT_EQ(to_string(l), "1 2 3 4 5");

    l.swap(p1, p1);
    ASSERT_EQ(to_string(l), "1 2 3 4 5");

    l.swap(p1, p2);
    ASSERT_EQ(to_string(l), "2 1 3 4 5");

    l.swap(p1, p2);
    ASSERT_EQ(to_string(l), "1 2 3 4 5");

    l.swap(p2, p1);
    ASSERT_EQ(to_string(l), "2 1 3 4 5");

    l.swap(p1, p2);
    ASSERT_EQ(to_string(l), "1 2 3 4 5");

    l.swap(p2, p4);
    ASSERT_EQ(to_string(l), "1 4 3 2 5");

    l.swap(p1, p5);
    ASSERT_EQ(to_string(l), "5 4 3 2 1");

    l.swap(p1, p4);
    ASSERT_EQ(to_string(l), "5 1 3 2 4");

    l.swap(p2, p4);
    ASSERT_EQ(to_string(l), "5 1 3 4 2");

    l.swap(p5, p4);
    ASSERT_EQ(to_string(l), "4 1 3 5 2");
}

TEST(ListTraitOperations, Clear) {
    LinkedList<int> l{};

    auto p1 = l.insertToTail(1);
    auto p2 = l.insertToTail(2);
    auto p3 = l.insertToTail(3);
    auto p4 = l.insertToTail(4);
    auto p5 = l.insertToTail(5);

    ASSERT_EQ(to_string(l), "1 2 3 4 5");

    auto _ = l.clear();
    ASSERT_EQ(to_string(l), "");
}

TEST(ListTraitOperations, Iterator) {
    LinkedList<int> l{};
    const auto     &cl = l;
    l.insertToTail(1);
    l.insertToTail(2);
    l.insertToTail(3);
    l.insertToTail(4);
    l.insertToTail(5);

    std::stringstream ss;

    for (auto e : l) { ss << e << " "; }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");

    for (auto e : cl) { ss << e << " "; }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");

    for (auto it = l.begin(); it != l.end(); ++it) { ss << *it << " "; }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");

    for (auto it = cl.begin(); it != cl.end(); ++it) { ss << *it << " "; }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");

    for (auto it = l.cbegin(); it != l.cend(); ++it) { ss << *it << " "; }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");

    for (auto it = cl.cbegin(); it != cl.cend(); ++it) { ss << *it << " "; }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");

    for (auto it = l.rbegin(); it != l.rend(); ++it) { ss << *it << " "; }
    ASSERT_EQ(ss.str(), "5 4 3 2 1 ");
    ss.str("");

    for (auto it = cl.rbegin(); it != cl.rend(); ++it) { ss << *it << " "; }
    ASSERT_EQ(ss.str(), "5 4 3 2 1 ");
    ss.str("");

    for (auto it = l.node_begin(); it != l.node_end(); ++it) {
        ss << it->value() << " ";
    }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");

    for (auto it = cl.node_begin(); it != cl.node_end(); ++it) {
        ss << it->value() << " ";
    }
    ASSERT_EQ(ss.str(), "1 2 3 4 5 ");
    ss.str("");
}

TEST(ListTraitOperations, Size) {
    LinkedList<int> l{};

    ASSERT_EQ(l.size(), 0);

    l.insertToTail(1);
    l.insertToTail(2);
    l.insertToTail(3);
    auto s = l.insertToTail(4);
    auto t = l.insertToTail(5);
    ASSERT_EQ(l.size(), 5);

    t->moveToPrev();
    ASSERT_EQ(l.size(), 5);

    t->removeFromList();
    ASSERT_EQ(l.size(), 4);

    auto foo = l.remove(l.find(2));
    ASSERT_EQ(l.size(), 3);

    l.insertBefore(s, 10086);
    ASSERT_EQ(l.size(), 4);

    auto bar = l.clear();
    ASSERT_EQ(l.size(), 0);
}

TEST(ListTraitOperations, InsertBefore) {
    LinkedList<int> l1{};
    LinkedList<int> l2{};

    auto p1 = l1.insertToTail(1);
    auto p2 = l1.insertToTail(2);
    auto p3 = l1.insertToTail(3);
    auto p4 = l1.insertToTail(4);
    auto p5 = l1.insertToTail(5);

    ASSERT_EQ(to_string(l1), "1 2 3 4 5");

    l1.insertBefore(p2, 111);
    ASSERT_EQ(to_string(l1), "1 111 2 3 4 5");

    l1.insertBefore(p5, 222);
    ASSERT_EQ(to_string(l1), "1 111 2 3 4 222 5");

    l1.insertBefore(p1, 333);
    ASSERT_EQ(to_string(l1), "333 1 111 2 3 4 222 5");

    auto other = l2.insertToTail(32);
    ASSERT_EQ(to_string(l2), "32");

    other->insertBefore(p1);
    ASSERT_EQ(to_string(l1), "333 32 1 111 2 3 4 222 5");
    ASSERT_EQ(to_string(l2), "");

    p3->insertBefore(p3);
    ASSERT_EQ(to_string(l1), "333 32 1 111 2 3 4 222 5");

    p3->insertBefore(p4);
    ASSERT_EQ(to_string(l1), "333 32 1 111 2 3 4 222 5");

    p3->insertBefore(p5);
    ASSERT_EQ(to_string(l1), "333 32 1 111 2 4 222 3 5");

    p5->insertBefore(other);
    ASSERT_EQ(to_string(l1), "333 5 32 1 111 2 4 222 3");
}

TEST(ListTraitOperations, InsertAfter) {
    LinkedList<int> l1{};
    LinkedList<int> l2{};

    auto p1 = l1.insertToTail(1);
    auto p2 = l1.insertToTail(2);
    auto p3 = l1.insertToTail(3);
    auto p4 = l1.insertToTail(4);
    auto p5 = l1.insertToTail(5);

    ASSERT_EQ(to_string(l1), "1 2 3 4 5");

    l1.insertAfter(p2, 111);
    ASSERT_EQ(to_string(l1), "1 2 111 3 4 5");

    auto last = l1.insertAfter(p5, 222);
    ASSERT_EQ(to_string(l1), "1 2 111 3 4 5 222");

    l1.insertAfter(p1, 333);
    ASSERT_EQ(to_string(l1), "1 333 2 111 3 4 5 222");

    auto other = l2.insertToTail(32);
    ASSERT_EQ(to_string(l2), "32");

    other->insertAfter(last);
    ASSERT_EQ(to_string(l1), "1 333 2 111 3 4 5 222 32");
    ASSERT_EQ(to_string(l2), "");

    p4->insertAfter(p4);
    ASSERT_EQ(to_string(l1), "1 333 2 111 3 4 5 222 32");

    p4->insertAfter(p3);
    ASSERT_EQ(to_string(l1), "1 333 2 111 3 4 5 222 32");

    p4->insertAfter(p1);
    ASSERT_EQ(to_string(l1), "1 4 333 2 111 3 5 222 32");

    p5->insertAfter(other);
    ASSERT_EQ(to_string(l1), "1 4 333 2 111 3 222 32 5");
}

TEST(ListTraitOperations, Find) {
    LinkedList<int> l{};

    auto p1 = l.insertToTail(1);
    auto p2 = l.insertToTail(2);
    auto p3 = l.insertToTail(3);
    auto p4 = l.insertToTail(4);
    auto p5 = l.insertToTail(5);

    ASSERT_EQ(l.find(999), l.node_end());
    ASSERT_EQ(l.find(1), l.node_begin());

    ASSERT_NE(l.find(1), l.node_end());
    ASSERT_NE(l.find(2), l.node_end());
    ASSERT_NE(l.find(3), l.node_end());
    ASSERT_NE(l.find(4), l.node_end());
    ASSERT_NE(l.find(5), l.node_end());

    ASSERT_EQ(l.find(1)->value(), 1);
    ASSERT_EQ(l.find(2)->value(), 2);
    ASSERT_EQ(l.find(3)->value(), 3);
    ASSERT_EQ(l.find(4)->value(), 4);
    ASSERT_EQ(l.find(5)->value(), 5);

    auto result = l.node_end();

    result = l.find_if([](const auto &node) {
        return node.value() == 1;
    });
    ASSERT_EQ(result, l.find(1));

    result = l.find_if([](const auto &node) {
        return node.value() == 2;
    });
    ASSERT_EQ(result, l.find(2));

    result = l.find_if([](const auto &node) {
        return node.value() == 3;
    });
    ASSERT_EQ(result, l.find(3));

    result = l.find_if([](const auto &node) {
        return node.value() == 4;
    });
    ASSERT_EQ(result, l.find(4));

    result = l.find_if([](const auto &node) {
        return node.value() == 5;
    });
    ASSERT_EQ(result, l.find(5));

    int sum = 0;
    result  = l.find_if([&sum](const auto &node) {
        sum += node.value();
        return sum > 5;
    });
    ASSERT_EQ(result, l.find(3));

    result = l.find_if([](const auto &node) {
        return node.value() % 2 == 0;
    });
    ASSERT_EQ(result, l.find(2));

    result = l.find_if([](const auto &) {
        return false;
    });
    ASSERT_EQ(result, l.node_end());
}

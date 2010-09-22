#include "tools_tester.h"
#include "Collections/array.h"

struct Item
{
    int a;
    char c;
    void *p;

    bool operator==(const Item &another)
    {
        return a == another.a && c == another.c && p == another.p;
    }
};

typedef CrashAndSqueeze::Collections::Array<Item> Array;

TEST(ArrayTest, Create)
{
    Array a;
    EXPECT_EQ(0, a.size());
}

TEST(ArrayTest, AddOne)
{
    Array a;
    Item item = {2, '3', 0};

    a.push_back(item);
    EXPECT_EQ(1, a.size());
    EXPECT_TRUE( item == a[0] );
    const Array &const_a = a;
    EXPECT_EQ(1, const_a.size());
    EXPECT_TRUE( item == const_a[0] );
}


TEST(ArrayTest, AddMany)
{
    const int NOT_MANY = 10;
    const int MANY = 100;
    Array a(NOT_MANY);
    Item items[MANY];

    for(int i = 0; i < MANY; ++i)
    {
        items[i].a = MANY - i;
        items[i].c = 'u';
        items[i].p = &items[i];

        a.push_back(items[i]);
    }

    EXPECT_EQ(MANY, a.size());
    for(int i = 0; i < MANY; ++i)
    {
        ASSERT_TRUE( items[i] == a[i] );
    }
}

TEST(ArrayTest, OutOfRange)
{
    Array a;
    Item item = {0};

    set_tester_err_callback();

    EXPECT_THROW( a[0], ToolsTesterException);
    EXPECT_THROW( a[-2], ToolsTesterException);

    a.push_back(item);

    EXPECT_THROW( a[1], ToolsTesterException);
    EXPECT_THROW( a[-2], ToolsTesterException);

    unset_tester_err_callback();
}

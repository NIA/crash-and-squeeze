#include "tools_tester.h"
#include "Collections/array.h"

struct Item
{
    int a;
    char c;
    void *p;

    bool operator==(const Item &another) const
    {
        return a == another.a && c == another.c && p == another.p;
    }
};

typedef CrashAndSqueeze::Collections::Array<Item> Array;

bool compare_items(Item const &a, Item const &b)
{
    return a == b;
}

bool compare_items_by_int(Item const &a, Item const &b)
{
    return a.a == b.a;
}

bool compare_items_like_no_one_cares(Item const &a, Item const &b)
{
    a; b; // ignore unreferenced
    return true;
}

TEST(ArrayTest, Create)
{
    Array a;
    EXPECT_EQ(0, a.size());
}

TEST(ArrayTest, BadCreate)
{
    set_tester_err_callback();
    
    EXPECT_THROW( Array(-1), ToolsTesterException);

    unset_tester_err_callback();
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

TEST(ArrayTest, CreateOne)
{
    Array a;
    Item item = {2, '3', 0};

    a.create_item() = item;
    EXPECT_EQ(1, a.size());
    EXPECT_TRUE( item == a[0] );
}

TEST(ArrayTest, AddOneToLazy)
{
    Array a(0);
    Item item = {2, '3', 0};

    a.push_back(item);
    EXPECT_EQ(1, a.size());
    EXPECT_TRUE( item == a[0] );
}

TEST(ArrayTest, AddManyAndIndexOf)
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
        ASSERT_EQ( i, a.index_of(a[i], compare_items) );
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

TEST(ArrayTest, IndexOfInEmpty)
{
    Array a;
    Item item = {0};

    EXPECT_EQ(Array::ITEM_NOT_FOUND_INDEX, a.index_of(item, compare_items));
}

TEST(ArrayTest, IndexOfNotExisting)
{
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    Item item = {1};

    EXPECT_EQ(Array::ITEM_NOT_FOUND_INDEX, a.index_of(item, compare_items));
}

TEST(ArrayTest, IndexOfOneOfTwo)
{
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    Item item = {1};
    a.push_back(item);
    a.push_back(item);

    EXPECT_EQ(1, a.index_of(item, compare_items));
}

TEST(ArrayTest, IndexOfWithFunction)
{
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    Item item = {1};
    a.push_back(item);
    a.push_back(some_boring_item);
    
    // change item we are looking for...
    item.p = &item.p;

    // ...but it still can be found
    EXPECT_EQ(1, a.index_of(item, compare_items_by_int));
}

TEST(ArrayTest, IndexOfWithStupidFunction)
{
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);
    
    Item item = {1};

    EXPECT_EQ(0, a.index_of(item, compare_items_like_no_one_cares));
}

TEST(ArrayTest, FindOrAdd_Find)
{
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);
    Item item = {1};
    a.push_back(item);
    a.push_back(some_boring_item);
    int size = a.size();

    EXPECT_TRUE(a[2] == a.find_or_add(item, compare_items));
    // nothing added
    EXPECT_EQ(size, a.size());
}

TEST(ArrayTest, FindOrAdd_Add)
{
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);
    Item item = {1};

    int size = a.size();

    EXPECT_TRUE(a[size] == a.find_or_add(item, compare_items));
    EXPECT_TRUE(item == a[size]);
    // nothing added
    EXPECT_EQ(size + 1, a.size());
}

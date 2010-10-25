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

TEST(ArrayTest, BoundaryCheck)
{
    set_tester_err_callback();

    const int SIZE = 3;
    Array a(SIZE);
    a.create_items(SIZE);
    
    EXPECT_THROW( a[-1], ToolsTesterException );
    EXPECT_NO_THROW( a[SIZE/2] );
    EXPECT_THROW( a[SIZE], ToolsTesterException );

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


TEST(ArrayTest, CreateItems)
{
    Array a;
    Item item = {2, '3', 0};
    a.push_back(item);

    a.create_items(10);
    EXPECT_TRUE( item == a[0] );
    EXPECT_EQ( 11, a.size() );
}

TEST(ArrayTest, CreateManyItems)
{
    const int NOT_MANY = 10;
    const int MANY = 100;
    Array a(NOT_MANY);
    Item item = {2, '3', 0};
    a.push_back(item);

    a.create_items(MANY - 1);

    EXPECT_EQ(MANY, a.size());
    EXPECT_TRUE( item == a[0] );
}

class CheckLifeCycle
{
    static int destructed;
    static int constructed;

public:
    static void reset() { destructed = constructed = 0; }
    static int get_destructed() { return destructed; }
    static int get_constructed() { return constructed; }
    
    CheckLifeCycle() { ++constructed; }
    ~CheckLifeCycle() { ++destructed; }
};
int CheckLifeCycle::destructed = 0;
int CheckLifeCycle::constructed = 0;

typedef CrashAndSqueeze::Collections::Array<CheckLifeCycle> CheckLifeCycleArray;

TEST(ArrayTest, ShouldNotBreakLifeCycle)
{
    const int NOT_MANY = 1;
    const int MANY = 2;

    CheckLifeCycle::reset();
    CheckLifeCycleArray * a = new CheckLifeCycleArray(NOT_MANY);
    EXPECT_EQ( 0, CheckLifeCycle::get_constructed() );
    
    CheckLifeCycle::reset();
    
    for(int i = 0; i < MANY; ++i)
    {
        a->create_item();
    }

    EXPECT_EQ( MANY, CheckLifeCycle::get_constructed() );
    EXPECT_EQ( 0, CheckLifeCycle::get_destructed() );

    delete a;
    EXPECT_EQ( MANY, CheckLifeCycle::get_destructed() );
}

TEST(ArrayTest, ShouldNotBreakLifeCycleWithBulkCreate)
{
    const int NOT_MANY = 10;
    const int MANY = 20;

    CheckLifeCycleArray * a = new CheckLifeCycleArray(NOT_MANY);
    CheckLifeCycle::reset();
    
    a->create_items(NOT_MANY);
    EXPECT_EQ( NOT_MANY, CheckLifeCycle::get_constructed() );

    CheckLifeCycle::reset();
    
    a->create_items(MANY - NOT_MANY);

    EXPECT_EQ( MANY - NOT_MANY, CheckLifeCycle::get_constructed() );
    EXPECT_EQ( 0, CheckLifeCycle::get_destructed() );

    CheckLifeCycle::reset();

    delete a;
    EXPECT_EQ( MANY, CheckLifeCycle::get_destructed() );
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

TEST(ArrayTest, Freeze)
{
    set_tester_err_callback();
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);

    a.freeze();
    
    EXPECT_TRUE( a.is_frozen() );
    EXPECT_THROW( a.push_back(some_boring_item), ToolsTesterException );
    EXPECT_THROW( a.create_item(), ToolsTesterException );
    unset_tester_err_callback();
}

TEST(ArrayTest, FindOrAddToFrozen)
{
    set_tester_err_callback();
    Array a;
    Item some_boring_item = {0};
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);
    a.push_back(some_boring_item);
    Item item = {1};

    a.freeze();

    EXPECT_NO_THROW( a.find_or_add(some_boring_item, compare_items) );
    EXPECT_THROW( a.find_or_add(item, compare_items), ToolsTesterException );
    set_tester_err_callback();
}

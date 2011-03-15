#include "tools_tester.h"
#include "Parallel/task_queue.h"

using namespace CrashAndSqueeze::Parallel;

class DummyTask : public AbstractTask
{
public:
    virtual void execute() { }
};

const int SIZE = 10;

TEST(TaskQueueTest, Init)
{
    TaskQueue tq(SIZE);

    EXPECT_TRUE(tq.is_empty());
    EXPECT_FALSE(tq.is_full());
}

TEST(TaskQueueTest, Push)
{
    TaskQueue tq(SIZE);
    DummyTask t;
    tq.push(&t);

    EXPECT_FALSE(tq.is_empty());
    EXPECT_FALSE(tq.is_full());
}

TEST(TaskQueueTest, PushNPop)
{
    TaskQueue tq(SIZE);
    DummyTask t1, t2;
    tq.push(&t1);
    tq.push(&t2);
    EXPECT_EQ(&t1, tq.pop());
    EXPECT_EQ(&t2, tq.pop());
    EXPECT_TRUE(tq.is_empty());
}

TEST(TaskQueueTest, PopFromEmpty)
{
    set_tester_err_callback();

    TaskQueue tq(SIZE);
    EXPECT_THROW( tq.pop(), ToolsTesterException );

    unset_tester_err_callback();
}

TEST(TaskQueueTest, PushToFull)
{
    set_tester_err_callback();

    TaskQueue tq(1);
    DummyTask t1, t2;
    tq.push(&t1);

    EXPECT_THROW( tq.push(&t2), ToolsTesterException );

    unset_tester_err_callback();
}

TEST(TaskQueueTest, Reset)
{
    TaskQueue tq(1);
    DummyTask t1;
    tq.push(&t1);
    tq.pop();

    // Here the queue is empty, but it is "full" because adding index has reached the end
    EXPECT_TRUE(tq.is_empty());
    EXPECT_TRUE(tq.is_full());
    tq.reset();
    // After we reset() the queue, everything is ok: we can add tasks again
    EXPECT_TRUE(tq.is_empty());
    EXPECT_FALSE(tq.is_full());
}
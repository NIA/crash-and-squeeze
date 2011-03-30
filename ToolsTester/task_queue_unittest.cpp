#include "tools_tester.h"
#include "Parallel/task_queue.h"
#include "Parallel/single_thread_prim.h"

using namespace CrashAndSqueeze::Parallel;

class DummyTask : public AbstractTask
{
protected:
    virtual void execute() { }
};

class TaskQueueTest : public ::testing::Test
{
protected:
    SingleThreadFactory factory;
    DummyTask t1, t2;

    static const int TQ_SIZE = 10;
    TaskQueue *tq;
    TaskQueue *small_queue;

    virtual void SetUp()
    {
        set_tester_err_callback();
        tq = new TaskQueue(TQ_SIZE, &factory);
        small_queue = new TaskQueue(1, &factory);
    }

    virtual void TearDown()
    {
        delete tq;
        delete small_queue;
        unset_tester_err_callback();
    }
};

TEST_F(TaskQueueTest, Init)
{
    EXPECT_TRUE(tq->is_empty());
    EXPECT_FALSE(tq->is_full());
}

TEST_F(TaskQueueTest, Push)
{
    tq->push(&t1);

    EXPECT_FALSE(tq->is_empty());
    EXPECT_FALSE(tq->is_full());
}

TEST_F(TaskQueueTest, PushNPop)
{
    tq->push(&t1);
    tq->push(&t2);
    EXPECT_EQ(&t1, tq->pop());
    EXPECT_EQ(&t2, tq->pop());
    EXPECT_TRUE(tq->is_empty());
}

TEST_F(TaskQueueTest, PopFromEmpty)
{
    EXPECT_EQ( NULL, tq->pop() );
}

TEST_F(TaskQueueTest, PushToFull)
{
    small_queue->push(&t1);
    EXPECT_THROW( small_queue->push(&t2), ToolsTesterException );
}

TEST_F(TaskQueueTest, Reset)
{
    small_queue->push(&t1);
    small_queue->pop()->complete();

    // Here the queue is empty, but it is "full" because adding index has reached the end
    EXPECT_TRUE(small_queue->is_empty());
    EXPECT_TRUE(small_queue->is_full());
    small_queue->reset();
    // After we reset() the queue, popped tasks return and become incomplete again
    EXPECT_FALSE(small_queue->is_empty());
    EXPECT_TRUE(small_queue->is_full());
    EXPECT_FALSE(small_queue->pop()->is_complete());
}

TEST_F(TaskQueueTest, Clear)
{
    tq->push(&t1);
    tq->clear();

    EXPECT_TRUE(tq->is_empty());
    EXPECT_EQ( NULL, tq->pop() );
}
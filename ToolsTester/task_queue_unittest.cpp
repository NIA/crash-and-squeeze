#include "tools_tester.h"
#include "Parallel/task_queue.h"

using namespace CrashAndSqueeze::Parallel;

class DummyTask : public AbstractTask
{
public:
    virtual void execute() { }
};

// This implementation of ILock makes the test fail
// if it is locked again. Good for avoiding stupid deadlocks
// in a single thread
class SingleThreadLock : public ILock
{
private:
    bool locked;
public:
    SingleThreadLock() : locked(false) {}
    virtual void lock()
    {
        ASSERT_FALSE(locked) << "Unexpected attempt to lock already locked lock!";
        locked = true;
    }
    virtual void unlock()
    {
        locked = false;
    }
};

// A factory for SingleThreadLock
class STLockFactory : public ILockFactory
{
public:
    ILock * create_lock()
    {
        return new SingleThreadLock();
    }
    void destroy_lock(ILock * lock)
    {
        delete lock;
    }
};

class TaskQueueTest : public ::testing::Test
{
protected:
    STLockFactory factory;
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
    EXPECT_THROW( tq->pop(), ToolsTesterException );
}

TEST_F(TaskQueueTest, PushToFull)
{
    small_queue->push(&t1);
    EXPECT_THROW( small_queue->push(&t2), ToolsTesterException );
}

TEST_F(TaskQueueTest, Reset)
{
    small_queue->push(&t1);
    small_queue->pop();

    // Here the queue is empty, but it is "full" because adding index has reached the end
    EXPECT_TRUE(small_queue->is_empty());
    EXPECT_TRUE(small_queue->is_full());
    small_queue->reset();
    // After we reset() the queue, everything is ok: we can add tasks again
    EXPECT_TRUE(small_queue->is_empty());
    EXPECT_FALSE(small_queue->is_full());
}
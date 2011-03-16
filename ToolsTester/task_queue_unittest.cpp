#include "tools_tester.h"
#include "Parallel/task_queue.h"

using namespace CrashAndSqueeze::Parallel;

class DummyTask : public AbstractTask
{
protected:
    virtual void execute() { }
};

// This implementation of ILock makes the test fail
// if it is locked again. Good for avoiding stupid deadlocks
// inside a single thread
class SingleThreadLock : public ILock
{
private:
    bool is_locked;
public:
    SingleThreadLock() : is_locked(false) {}
    virtual void lock()
    {
        ASSERT_FALSE(is_locked) << "Unexpected attempt to lock already locked lock from the same thread!";
        is_locked = true;
    }
    virtual void unlock() { is_locked = false; }
};

// This implementation of IEvent makes the test fail
// if wait() is called when the event not set, because
// in a single-threaded application it will be a deadlock
class SingleThreadEvent : public IEvent
{
private:
    bool is_set;
public:
    SingleThreadEvent(bool initially_set) : is_set(initially_set) {}
    
    virtual void set()   { is_set = true;  }
    virtual void unset() { is_set = false; }

    virtual void wait()
    {
        ASSERT_TRUE(is_set) << "Unexpected attempt to wait for unset event from the same thread!";
    }
};

// A factory for SingleThreadLock
class STPrimFactory : public IPrimFactory
{
public:
    virtual ILock * create_lock() { return new SingleThreadLock(); }
    virtual void destroy_lock(ILock * lock) { delete lock; }
    
    virtual IEvent * create_event(bool initially_set = false) { return new SingleThreadEvent(initially_set); }
    virtual void destroy_event(IEvent * event) { delete event; }
};

class TaskQueueTest : public ::testing::Test
{
protected:
    STPrimFactory factory;
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

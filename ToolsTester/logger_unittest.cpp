#include "tools_tester.h"
#include "Logging/logger.h"

using namespace ::CrashAndSqueeze::Logging;

class LoggerTest : public ::testing::Test
{
protected:
    static bool was_some_callback_invoked;
    static const char* message_given;
    static const char* file_given;
    static int line_given;

public:
    static void some_callback(const char * message, const char * file, int line)
    {
        was_some_callback_invoked = true;
        message_given = message;
        file_given = file;
        line_given = line;
    }

protected:    
    CallbackAction some_action;

    Logger test_logger;

    virtual void SetUp()
    {
        was_some_callback_invoked = false;
        message_given = 0;
        file_given = 0;
        line_given = 0;

        test_logger.set_default_action(Logger::LOG);
        test_logger.set_default_action(Logger::WARNING);
        test_logger.set_default_action(Logger::ERROR);
    }

    void expect_invoked()
    {
        EXPECT_TRUE(was_some_callback_invoked);
        EXPECT_EQ("msg", message_given);
        EXPECT_EQ("file.cpp", file_given);
        EXPECT_EQ(123, line_given);
    }

    void expect_not_invoked()
    {
        EXPECT_FALSE(was_some_callback_invoked);
    }

    void expect_sets_and_gets(Logger::Level level)
    {
        test_logger.set_action(level, &some_action);
        EXPECT_EQ( &some_action, test_logger.get_action(level) );
    }

    virtual void TearDown()
    {
        test_logger.set_default_action(Logger::LOG);
        test_logger.set_default_action(Logger::WARNING);
        test_logger.set_default_action(Logger::ERROR);
    }

public:
    LoggerTest() : some_action(some_callback) { SetUp(); }
};

bool LoggerTest::was_some_callback_invoked = false;
const char* LoggerTest::message_given = 0;
const char* LoggerTest::file_given = 0;
int LoggerTest::line_given = 0;

TEST_F(LoggerTest, SetAndGet)
{
    {
        SCOPED_TRACE("Log");
        expect_sets_and_gets(Logger::LOG);
    }
    {
        SCOPED_TRACE("Warning");
        expect_sets_and_gets(Logger::WARNING);
    }
    {
        SCOPED_TRACE("Error");
        expect_sets_and_gets(Logger::ERROR);
    }
}

TEST_F(LoggerTest, Log)
{
    test_logger.set_action(Logger::LOG, &some_action);
    test_logger.log("msg", "file.cpp", 123);
    SCOPED_TRACE("Log");
    expect_invoked();
}

TEST_F(LoggerTest, Warning)
{
    test_logger.set_action(Logger::WARNING, &some_action);
    test_logger.warning("msg", "file.cpp", 123);
    SCOPED_TRACE("Warning");
    expect_invoked();
}

TEST_F(LoggerTest, Error)
{
    test_logger.set_action(Logger::ERROR, &some_action);
    test_logger.error("msg", "file.cpp", 123);
    SCOPED_TRACE("Error");
    expect_invoked();
}

TEST_F(LoggerTest, NoLog)
{
    test_logger.ignore(Logger::LOG);
    test_logger.log("msg");
    expect_not_invoked();
}

TEST_F(LoggerTest, NoWarning)
{
    test_logger.ignore(Logger::WARNING);
    test_logger.warning("msg");
    expect_not_invoked();
}

TEST_F(LoggerTest, NoError)
{
    test_logger.ignore(Logger::ERROR);
    test_logger.error("msg");
    expect_not_invoked();
}

class YetAnotherAction : public Action
{
    virtual void invoke(const char * message, const char * file, int line)
    {
        LoggerTest::some_callback(message, file, line);
    }
};

TEST_F(LoggerTest, PolymorphicLog)
{
    YetAnotherAction action;
    test_logger.set_action(Logger::LOG, &action);
    test_logger.log("msg", "file.cpp", 123);
    expect_invoked();
}

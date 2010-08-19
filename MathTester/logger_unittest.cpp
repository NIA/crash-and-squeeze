#include "math_tester.h"
#include "logger.h"

using namespace ::CrashAndSqueeze::Logging;

class LoggerTest : public ::testing::Test
{
protected:
    static bool was_some_callback_invoked;
    static const char* message_given;
    static const char* file_given;
    static int line_given;
    static void some_callback(const char * message, const char * file, int line)
    {
        was_some_callback_invoked = true;
        message_given = message;
        file_given = file;
        line_given = line;
    }

    Logger test_logger;

    virtual void SetUp()
    {
        was_some_callback_invoked = false;
        message_given = 0;
        file_given = 0;
        line_given = 0;
    }

    void check_results()
    {
        EXPECT_TRUE(was_some_callback_invoked);
        EXPECT_EQ("msg", message_given);
        EXPECT_EQ("file.cpp", file_given);
        EXPECT_EQ(123, line_given);
    }
};

bool LoggerTest::was_some_callback_invoked = false;
const char* LoggerTest::message_given = 0;
const char* LoggerTest::file_given = 0;
int LoggerTest::line_given = 0;


TEST_F(LoggerTest, Log)
{
    test_logger.log_callback = some_callback;
    test_logger.log("msg", "file.cpp", 123);
    SCOPED_TRACE("Log");
    check_results();
}

TEST_F(LoggerTest, Warning)
{
    test_logger.warning_callback = some_callback;
    test_logger.warning("msg", "file.cpp", 123);
    SCOPED_TRACE("Warning");
    check_results();
}

TEST_F(LoggerTest, Error)
{
    test_logger.error_callback = some_callback;
    test_logger.error("msg", "file.cpp", 123);
    SCOPED_TRACE("Error");
    check_results();
}

TEST_F(LoggerTest, NoLog)
{
    EXPECT_NO_THROW({
        test_logger.log_callback = 0;
        test_logger.log("msg", "file.cpp", 123);
    });
}

TEST_F(LoggerTest, NoWarning)
{
    EXPECT_NO_THROW({
        test_logger.warning_callback = 0;
        test_logger.warning("msg", "file.cpp", 123);
    });
}

TEST_F(LoggerTest, NoError)
{
    test_logger.error_callback = 0;
    EXPECT_THROW( test_logger.error("msg", "file.cpp", 123), ::CrashAndSqueeze::Logging::Error );
}

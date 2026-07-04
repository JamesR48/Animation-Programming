#ifndef CPPANIMPROGRAMMING_LOGGER_H
#define CPPANIMPROGRAMMING_LOGGER_H

#include <cstdio>

class Logger
{
public:
    /* log if input log level is equal or smaller to log level set */
    template <typename... Args>
    static void Log(const unsigned int LogLevel, Args ... args)
    {
        if (LogLevel <= mLogLevel)
        {
            std::printf(args ...);

            // Force output
            std::fflush(stdout);
        }
    }

    static void SetLogLevel(const unsigned int InLogLevel)
    {
        InLogLevel <= 9 ? mLogLevel = InLogLevel :
            mLogLevel = 9;
    }

private:
    static unsigned int mLogLevel;
};

#endif //CPPANIMPROGRAMMING_LOGGER_H

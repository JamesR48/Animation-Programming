#include "Timer.h"
#include "Logger.h"

void Timer::Start()
{
    if (mRunning)
    {
        Logger::Log(1, "%s error: timer already running\n", __FUNCTION__);
        return;
    }

    mRunning = true;
    mStartTime = std::chrono::steady_clock::now();
}

float Timer::Stop()
{
    if (!mRunning) {
        Logger::Log(1, "%s error: timer not running\n", __FUNCTION__);
        return 0;
    }
    mRunning = false;

    auto StopTime = std::chrono::steady_clock::now();
    float TimerMilliSeconds = std::chrono::duration_cast<std::chrono::microseconds>(StopTime - mStartTime).count() / 1000.0f;

    return TimerMilliSeconds;
}
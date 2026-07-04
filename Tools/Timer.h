#ifndef CPPANIMPROGRAMMING_TIMER_H
#define CPPANIMPROGRAMMING_TIMER_H

#include <chrono>

class Timer
{
public:
    void Start();
    /* stops timer and returns milliseconds since start, in microsecond resolution */
    float Stop();

private:
    bool mRunning = false;
    std::chrono::time_point<std::chrono::steady_clock> mStartTime{};
};

#endif //CPPANIMPROGRAMMING_TIMER_H

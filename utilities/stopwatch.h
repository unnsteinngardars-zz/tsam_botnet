#ifndef STOPWATCH_H
#define STOPWATCH_H
#include "../utilities/time_utilities.h"

class StopWatch
{
    private:
    time_t timer;
    public:
    void start();
    bool elapsed(int seconds);
    void reset();
};

#endif
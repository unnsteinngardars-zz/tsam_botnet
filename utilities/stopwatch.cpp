#include "stopwatch.h"

void StopWatch::start()
{
    time_utilities::set_timer(timer);
}
bool StopWatch::elapsed(int seconds)
{
    time_t now = time(NULL);
    int seconds_diff = time_utilities::get_time_in_seconds(timer, now);
    return seconds_diff >= seconds;
}
void StopWatch::reset()
{
    start();
}
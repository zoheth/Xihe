#include "timer.h"

namespace xihe
{
Timer::Timer() :
    start_time_{Clock::now()},
    previous_tick_{Clock::now()}
{
}

void Timer::start()
{
	if (!running_)
	{
		running_    = true;
		start_time_ = Clock::now();
	}
}

void Timer::lap()
{
	lapping_  = true;
	lap_time_ = Clock::now();
}

bool Timer::is_running() const
{
	return running_;
}
}        // namespace xihe

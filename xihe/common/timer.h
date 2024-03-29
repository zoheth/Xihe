#pragma once

#include <chrono>

namespace xihe
{
class Timer
{
  public:
	using Seconds      = std::ratio<1>;
	using Milliseconds = std::ratio<1, 1000>;
	using Microseconds = std::ratio<1, 1000000>;
	using Nanoseconds  = std::ratio<1, 1000000000>;

	// Configure
	using Clock             = std::chrono::steady_clock;
	using DefaultResolution = Seconds;

	Timer();

	virtual ~Timer() = default;

	/**
	 * @brief Starts the timer, elapsed() now returns the duration since start()
	 */
	void start();

	/**
	 * @brief Laps the timer, elapsed() now returns the duration since the last lap()
	 */
	void lap();

	/**
	 * @brief Stops the timer, elapsed() now returns 0
	 * @return The total execution time between `start()` and `stop()`
	 */
	template <typename T = DefaultResolution>
	double stop()
	{
		if (!running_)
		{
			return 0;
		}

		running_       = false;
		lapping_       = false;
		auto duration = std::chrono::duration<double, T>(Clock::now() - start_time_);
		start_time_    = Clock::now();
		lap_time_      = Clock::now();

		return duration.count();
	}

	/**
	 * @brief Calculates the time difference between now and when the timer was started
	 *        if lap() was called, then between now and when the timer was last lapped
	 * @return The duration between the two time points (default in seconds)
	 */
	template <typename T = DefaultResolution>
	double elapsed()
	{
		if (!running_)
		{
			return 0;
		}

		Clock::time_point start = start_time_;

		if (lapping_)
		{
			start = lap_time_;
		}

		return std::chrono::duration<double, T>(Clock::now() - start).count();
	}

	/**
	 * @brief Calculates the time difference between now and the last time this function was called
	 * @return The duration between the two time points (default in seconds)
	 */
	template <typename T = DefaultResolution>
	double tick()
	{
		auto now      = Clock::now();
		auto duration = std::chrono::duration<double, T>(now - previous_tick_);
		previous_tick_ = now;
		return duration.count();
	}

	/**
	 * @brief Check if the timer is running
	 */
	bool is_running() const;

  private:
	bool running_{false};

	bool lapping_{false};

	Clock::time_point start_time_;

	Clock::time_point lap_time_;

	Clock::time_point previous_tick_;
};
}        // namespace xihe

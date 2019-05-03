#pragma once

#include <chrono>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

inline void startTime(TimePoint* point) {
	*point = Clock::now();
}

template <typename T = double>
inline T deltaTime(const TimePoint& point) {
	return std::chrono::duration_cast<std::chrono::duration<T>>(Clock::now() - point).count();
}

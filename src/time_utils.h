#ifndef __GIT_HEATMAP_TIME_UTILS_H__
#define __GIT_HEATMAP_TIME_UTILS_H__

#include <chrono>
std::chrono::hours timezon_offset();
std::chrono::system_clock::time_point now();
std::chrono::system_clock::time_point monday_of_week(
    std::chrono::system_clock::time_point now);

#endif  // __GIT_HEATMAP_TIME_UTILS_H__

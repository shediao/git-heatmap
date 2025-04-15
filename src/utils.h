#ifndef __GIT_HEATMAP_UTILS_H__
#define __GIT_HEATMAP_UTILS_H__

#include <chrono>

constexpr static int MAX_DISPLAY_WEEKS = 52;

std::chrono::hours timezon_offset();

std::chrono::sys_days today();
std::chrono::sys_days monday(std::chrono::sys_days d = today());
std::chrono::sys_days sunday(std::chrono::sys_days d = today());

#endif  // __GIT_HEATMAP_TIME_UTILS_H__

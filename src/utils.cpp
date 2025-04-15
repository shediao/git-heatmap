#include "utils.h"

#include <chrono>

std::chrono::hours timezon_offset() {
    static auto ret = []() {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* gmtm = std::gmtime(&now_time_t);
        auto gm_time_t = std::mktime(gmtm);
        return std::chrono::hours((now_time_t - gm_time_t) / 3600);
    }();
    return ret;
}

std::chrono::sys_days today() {
    return std::chrono::floor<std::chrono::days>(
        std::chrono::system_clock::now() + timezon_offset());
}
std::chrono::sys_days monday(std::chrono::sys_days d) {
    auto this_weekday = std::chrono::weekday(d);
    auto week_index = this_weekday.c_encoding();
    return std::chrono::sys_days(d) -
           std::chrono::days((week_index == 0 ? 7 : week_index) - 1);
}
std::chrono::sys_days sunday(std::chrono::sys_days d) {
    return monday(d) + std::chrono::days(6);
}

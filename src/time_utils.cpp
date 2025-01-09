#include "time_utils.h"

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

std::chrono::system_clock::time_point now() {
    return std::chrono::system_clock::now() + timezon_offset();
}

std::chrono::system_clock::time_point monday_of_week(
    std::chrono::system_clock::time_point now) {
    std::chrono::year_month_day now_day =
        std::chrono::floor<std::chrono::days>(now);
    auto now_weekday = std::chrono::weekday(now_day);
    return std::chrono::sys_days(now_day) -
           std::chrono::days(now_weekday.c_encoding() == 0 ? 6 : 1);
}

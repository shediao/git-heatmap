#include "git_heatmap.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "debug.h"

// 定义颜色方案
const std::map<std::string, HeatMap::ColorScheme> HeatMap::schemes = {
    {"default", {"#333333", "#9be9a8", "#40c463", "#30a14e", "#216e39"}},
    {"github", {"#ebedf0", "#9be9a8", "#40c463", "#30a14e", "#216e39"}},
    {"vibrant", {"#333333", "#F9ED69", "#F08A5D", "#B83B5E", "#6A2C70"}},
    {"blackwhite", {"#333333", "#707070", "#a0a0a0", "#d0d0d0", "#ffffff"}},
    {"dracula", {"#282a36", "#50fa7b", "#ff79c6", "#bd93f9", "#6272a4"}},
    {"north", {"#333333", "#DBE2EF", "#3282B8", "#3F72AF", "#112D4E"}},
    {"gold", {"#333333", "#FDEEDC", "#FFD8A9", "#F1A661", "#E38B29"}},
    {"sunset", {"#333333", "#F67280", "#C06C84", "#6C5B7B", "#355C7D"}},
    {"mint", {"#333333", "#5AFEAD", "#21B475", "#1D794B", "#294B36"}}};

// 其他静态成员定义
const std::string HeatMap::info = "\033[38;2;100;100;100m";
const std::string HeatMap::reset = "\033[0m";
const std::map<std::string, std::pair<const char*, const char*>>
    HeatMap::blocks = {{"square", {"◼︎", "◼︎"}},  {"block", {"█", "█"}},
                       {"dot", {"•", "•"}},     {"fisheye", {"◉", "●"}},
                       {"diamond", {"♦︎", "♦︎"}}, {"plus", {"✚", "•"}}};

// 将16进制颜色转换为ANSI转义序列
std::string HeatMap::rgb_color(const char* hex) const {
    unsigned int r, g, b;
    sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" +
           std::to_string(b) + "m";
}

std::string HeatMap::get_level_color(size_t level) const {
    return rgb_color(color_scheme[level]);
}

HeatMap::HeatMap(const std::map<std::chrono::sys_days, int>& commits,
                 const std::string& email, const std::string& scheme,
                 const std::chrono::sys_days& start,
                 const std::chrono::sys_days& end)
    : author_email(email),
      start_days(start),
      end_days(end),
      color_scheme(HeatMap::get_scheme(scheme)) {
    build_grid(commits);
}

std::string HeatMap::format_date(const std::chrono::sys_days& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&time_t);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

int HeatMap::get_terminal_width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    } else {
        return 84;
    }
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col == 0 ? 256 : w.ws_col;
#endif
}

std::string HeatMap::get_color(int count) const {
    if (count == 0) {
        return get_level_color(0);
    }
    if (count <= 2) {
        return get_level_color(1);
    }
    if (count <= 5) {
        return get_level_color(2);
    }
    if (count <= 10) {
        return get_level_color(3);
    }
    return get_level_color(4);
}

void HeatMap::build_grid(const std::map<std::chrono::sys_days, int>& commits) {
    DEBUG_LOG("Building calendar grid");
    DEBUG_LOG("Calendar period: " << format_date(start_days) << " to "
                                  << format_date(end_days));

    size_t display_weeks = max_display_weeks();
    // 获取最早和最晚的提交时间
    DEBUG_LOG("Display weeks: " << display_weeks);

    auto monday = [](std::chrono::sys_days d) {
        auto d_weekday = std::chrono::weekday(d);
        auto d_weekday_inde = d_weekday.c_encoding();
        return std::chrono::sys_days(
            d -
            std::chrono::days(d_weekday_inde == 0 ? 6 : d_weekday_inde - 1));
    };
    auto sunday = [](std::chrono::sys_days d) {
        auto d_weekday = std::chrono::weekday(d);
        auto d_weekday_inde = d_weekday.c_encoding();
        return d +
               std::chrono::days(d_weekday_inde == 0 ? 0 : 7 - d_weekday_inde);
    };
    start_days = monday(end_days - std::chrono::days((display_weeks - 1) * 7));
    end_days = sunday(end_days);
    DEBUG_LOG("Grid start day: " << std::format(
                  "{:%Y-%m-%d}", std::chrono::sys_days{start_days}));

    // 初始化网格
    grid.resize(7, std::vector<Cell>(display_weeks, {0, get_level_color(0)}));

    // 计算月份标签
    month_label = std::string(display_weeks * 2, ' ');
    for (size_t i = 0; i < display_weeks; ++i) {
        auto last_sunday_day = std::chrono::year_month_day(
            start_days + std::chrono::days(i * 7 - 1));
        auto current_sunday_day = std::chrono::year_month_day(
            start_days + std::chrono::days(i * 7 + 6));

        if (last_sunday_day.month() != current_sunday_day.month()) {
            unsigned int month =
                static_cast<unsigned int>(current_sunday_day.month());
            month_label[i * 2] = month < 10 ? '0' : '1';
            month_label[i * 2 + 1] = '0' + (month < 10 ? month : month % 10);
        }
    }

    // 记录处理的提交数
    int total_commits = 0;
    int displayed_commits = 0;

    for (const auto& [commit_time, count] : commits) {
        total_commits++;
        if (commit_time < start_days || commit_time > end_days) {
            DEBUG_LOG("Skipping commit on "
                      << std::format("{:%Y-%m-%d}", commit_time)
                      << " (outside display range)");
            continue;
        }

        auto week_diff =
            (commit_time - std::chrono::sys_days(start_days)).count() / 7;
        auto current_weekday = std::chrono::weekday(commit_time);
        auto weekday_index = current_weekday.c_encoding() == 0
                                 ? 6
                                 : current_weekday.c_encoding() - 1;

        DEBUG_LOG("Processing commit on "
                  << std::format("{:%Y-%m-%d}", commit_time) << " week_diff: "
                  << week_diff << " weekday_index: " << weekday_index
                  << " count: " << count);

        if (week_diff >= 0 && week_diff < static_cast<int>(display_weeks)) {
            grid[weekday_index][week_diff].count += count;
            grid[weekday_index][week_diff].color =
                get_color(grid[weekday_index][week_diff].count);
            displayed_commits++;
        } else {
            DEBUG_LOG("Invalid grid position: week_diff="
                      << week_diff << " weekday_index=" << weekday_index);
        }
    }

    DEBUG_LOG("Total commits processed: " << total_commits);
    DEBUG_LOG("Commits displayed: " << displayed_commits);
    DEBUG_LOG("Grid dimensions: " << grid.size() << "x" << grid[0].size());
}

void HeatMap::display() const {
    DEBUG_LOG("Displaying calendar");
    std::stringstream output;

    output << "\n"
           << HeatMap::info << std::string(week_label[0].size(), ' ')
           << month_label << HeatMap::reset << "\n";

    auto total_commits = 0;

    const char* full = get_block(GetArgs().glyph).first;
    const char* empty = get_block(GetArgs().glyph).second;
    // 显示热力图
    for (size_t row = 0; row < grid.size(); ++row) {
        output << HeatMap::info << week_label[row] << HeatMap::reset;
        for (size_t col = 0; col < grid[row].size(); ++col) {
            output << " " << grid[row][col].color
                   << (grid[row][col].count > 0 ? full : empty)
                   << HeatMap::reset;
            total_commits += grid[row][col].count;
        }
        output << "\n";
    }

    // 显示颜色图例
    output << "\n"
           << HeatMap::info << "Author: " << HeatMap::reset
           << (author_email.empty() ? "*" : author_email) << ", "
           << "Commits: " << total_commits << " " << HeatMap::reset;
    output << get_level_color(0) << empty << HeatMap::reset << " 0 ";
    output << get_level_color(1) << full << HeatMap::reset << " 1~2 ";
    output << get_level_color(2) << full << HeatMap::reset << " 3~5 ";
    output << get_level_color(3) << full << HeatMap::reset << " 6~10 ";
    output << get_level_color(4) << full << HeatMap::reset << " >10\n";
    std::cout << output.str() << std::flush;
}

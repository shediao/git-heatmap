#ifndef __GIT_HEATMAP_COMMIT_CALENDAR_H__
#define __GIT_HEATMAP_COMMIT_CALENDAR_H__
#include <algorithm>
#include <array>
#include <chrono>
#include <map>
#include <string>
#include <vector>

class HeatMap {
    using ColorScheme = std::array<const char*, 5>;

   public:
    explicit HeatMap(const std::map<std::chrono::sys_days, int>& commits,
                     const std::string& author_email, const std::string& scheme,
                     const std::chrono::sys_days& start_date,
                     const std::chrono::sys_days& end_date);
    void display() const;

    // 添加一个静态方法来计算可显示的周数
    static size_t max_display_weeks(int label_width = 3) {
        int max_terminal_weeks = (get_terminal_width() - label_width - 1) / 2;
        return static_cast<size_t>(std::clamp(max_terminal_weeks, 12, 52));
    }

    // 根据颜色值生成ANSI转义序列
    std::string rgb_color(const char* hex) const;

    // 获取对应级别的颜色
    std::string get_level_color(size_t level) const;

    // 预定义的颜色方案
    static const std::map<std::string, ColorScheme> schemes;
    static const ColorScheme& get_scheme(const std::string& name) {
        auto it = schemes.find(name);
        if (it == schemes.end()) {
            it = schemes.begin();
        }
        return it->second;
    }
    static const std::pair<const char*, const char*>& get_block(
        const std::string& name) {
        auto it = blocks.find(name);
        if (it == blocks.end()) {
            it = blocks.begin();
        }
        return it->second;
    }

    static const std::string info;   // 信息文字颜色
    static const std::string reset;  // 重置颜色
    static const std::map<std::string, std::pair<const char*, const char*>>
        blocks;  // 填充块字符

   private:
    struct Cell {
        int count = 0;
        std::string color;
    };

    std::vector<std::vector<Cell>> grid;
    std::string author_email;
    std::string month_label;
    std::vector<std::string> week_label{"Mon", "Tue", "Wed", "Thu",
                                        "Fri", "Sat", "Sun"};
    std::chrono::sys_days start_days;
    std::chrono::sys_days end_days;
    ColorScheme color_scheme;

    std::string get_color(int count) const;
    void build_grid(const std::map<std::chrono::sys_days, int>& commits);
    static int get_terminal_width();
    static std::string format_date(const std::chrono::sys_days& time);
};
#endif  // __GIT_HEATMAP_COMMIT_CALENDAR_H__

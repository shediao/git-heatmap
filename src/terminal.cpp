
#include "terminal.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <cassert>
#include <iostream>
#include <map>

#include "utils.h"

static std::vector<std::string> week_label{"Mon", "Tue", "Wed", "Thu",
                                           "Fri", "Sat", "Sun"};

[[maybe_unused]] static CommitNumberLevel get_commit_number_level(int count) {
    if (count == 0) {
        return CommitNumberLevel::LEVEL0;
    }
    if (2 >= count) {
        return CommitNumberLevel::LEVEL1;
    }
    if (5 >= count) {
        return CommitNumberLevel::LEVEL2;
    }
    if (10 >= count) {
        return CommitNumberLevel::LEVEL3;
    }
    return CommitNumberLevel::LEVEL4;
}

[[maybe_unused]] static int color_string_length(std::string const& str) {
    int len = 0;

    auto i = str.begin();
    auto end = str.end();
    while (i < end) {
        if (*i == 033 && (i + 1 < end) && '[' == *(i + 1)) {
            auto j = i + 2;
            while (j < end) {
                if (*j == 'm') {
                    i = j + 1;
                    break;
                } else {
                    ++j;
                }
            }
        } else {
            unsigned char c = *i;
            if ((c & 0x80) == 0) {
                // 1-byte character (ASCII)
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                // 2-byte character
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                // 3-byte character
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                // 4-byte character
                i += 4;
            } else {
                // Invalid UTF-8 byte sequence
                throw std::runtime_error("Invalid UTF-8 character detected");
            }
            ++len;
        }
    }
    return len;
}

ColorScheme::ColorScheme(std::string const& color)
    : current_color(ColorScheme::default_schemes.at(color)) {}

std::map<std::string, ColorScheme::Scheme> const ColorScheme::default_schemes =
    {{"default", {"#333333", "#9be9a8", "#40c463", "#30a14e", "#216e39"}},
     {"github", {"#ebedf0", "#9be9a8", "#40c463", "#30a14e", "#216e39"}},
     {"vibrant", {"#333333", "#F9ED69", "#F08A5D", "#B83B5E", "#6A2C70"}},
     {"blackwhite", {"#333333", "#707070", "#a0a0a0", "#d0d0d0", "#ffffff"}},
     {"dracula", {"#282a36", "#50fa7b", "#ff79c6", "#bd93f9", "#6272a4"}},
     {"north", {"#333333", "#DBE2EF", "#3282B8", "#3F72AF", "#112D4E"}},
     {"gold", {"#333333", "#FDEEDC", "#FFD8A9", "#F1A661", "#E38B29"}},
     {"sunset", {"#333333", "#F67280", "#C06C84", "#6C5B7B", "#355C7D"}},
     {"mint", {"#333333", "#5AFEAD", "#21B475", "#1D794B", "#294B36"}}};

const std::string ColorScheme::info = "\033[38;2;100;100;100m";
const std::string ColorScheme::reset = "\033[0m";

const std::map<std::string, std::pair<const char*, const char*>>
    ColorScheme::blocks = {{"square", {"◼︎", "◼︎"}},  {"block", {"█", "█"}},
                           {"dot", {"•", "•"}},     {"fisheye", {"◉", "●"}},
                           {"diamond", {"♦︎", "♦︎"}}, {"plus", {"✚", "•"}}};

// 将16进制颜色转换为ANSI转义序列
static std::string rgb_color(const char* hex) {
    unsigned int r, g, b;
    sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" +
           std::to_string(b) + "m";
}

std::string ColorScheme::level_color(CommitNumberLevel level) const {
    return rgb_color(current_color[static_cast<int>(level)]);
}

Terminal::Terminal(std::string const& color_scheme, std::string const& glyph)
    : color_scheme(color_scheme), glyph{ColorScheme::blocks.at(glyph)} {}

int Terminal::columns() const {
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
    return 0;
}
std::string Terminal::info_color() const { return ColorScheme::info; }
std::string Terminal::reset_color() const { return ColorScheme::reset; }
std::string Terminal::level_color(CommitNumberLevel level) const {
    return color_scheme.level_color(level);
}

static std::string make_month_lable(
    std::vector<std::pair<const std::chrono::sys_days, int>> commits) {
    std::string month_lable(commits.size() / 7 * 2, ' ');
    for (auto i = begin(commits); i < end(commits); i = i + 7) {
        std::chrono::year_month_day s = i->first;
        std::chrono::year_month_day e = (i + 6)->first;
        if (s.month() != e.month() ||
            1 == (static_cast<unsigned int>(s.day()))) {
            auto m = static_cast<unsigned int>(e.month());
            if (m >= 10) {
                month_lable[(i - begin(commits)) / 7 * 2] = '1';
            }
            month_lable[(i - begin(commits)) / 7 * 2 + 1] = '0' + (m % 10);
        }
    }
    return month_lable;
}

static std::string make_footer_lable(
    std::string const& email,
    std::vector<std::pair<const std::chrono::sys_days, int>> commits,
    ColorScheme const& color_scheme,
    std::pair<const char*, const char*> glyph) {
    std::stringstream output;
    auto [full, empty] = glyph;
    output << color_scheme.level_color(CommitNumberLevel::LEVEL0) << empty
           << color_scheme.reset << " 0 ";
    output << color_scheme.level_color(CommitNumberLevel::LEVEL1) << full
           << color_scheme.reset << " 1~2 ";
    output << color_scheme.level_color(CommitNumberLevel::LEVEL2) << full
           << color_scheme.reset << " 3~5 ";
    output << color_scheme.level_color(CommitNumberLevel::LEVEL3) << full
           << color_scheme.reset << " 6~10 ";
    output << color_scheme.level_color(CommitNumberLevel::LEVEL4) << full
           << color_scheme.reset << " >10";

    std::string footer_lable_right = output.str();

    auto total = 0;
    for (auto const& c : commits) {
        total += c.second;
    }
    std::string footer_lable_left =
        "Author: " + email + ", commits: " + std::to_string(total);

    // auto footer_lable_right_len = color_string_length(footer_info);
    auto footer_lable_right_len = 28;

    auto spaces =
        std::string(MAX_DISPLAY_WEEKS * 2 - footer_lable_left.length() -
                        footer_lable_right_len,
                    ' ');

    return footer_lable_left + spaces + footer_lable_right;
}

void Terminal::display(
    std::vector<std::pair<const std::chrono::sys_days, int>> commits) {
    assert((commits.size() % 7) == 0);
    assert((commits.size() / 7) == MAX_DISPLAY_WEEKS);

    auto month_lable =
        color_scheme.info + make_month_lable(commits) + color_scheme.reset;

    std::ostringstream output;
    output << "   " << month_lable << "\n";

    auto [full, empty] = glyph;

    for (int i = 0; i < 7; i++) {
        output << color_scheme.info << week_label[i] << color_scheme.reset;
        for (int j = 0; j < (int)commits.size() / 7; j++) {
            auto c = commits.begin() + i + j * 7;
            if (c->second > 0) {
                output << " "
                       << color_scheme.level_color(
                              get_commit_number_level(c->second))
                       << full << color_scheme.reset;
            } else {
                output << " "
                       << color_scheme.level_color(
                              get_commit_number_level(c->second))
                       << empty << color_scheme.reset;
            }
        }
        output << "\n";
    }

    auto footer_lable = color_scheme.info +
                        make_footer_lable("me", commits, color_scheme, glyph) +
                        color_scheme.reset;

    output << "   " << footer_lable << "\n";

    std::cout.flush();
    std::cout << output.str();
    std::cout.flush();
    return;
}

std::string Terminal::show_example(std::string const& color_scheme,
                                   std::string const& glyph) {
    ColorScheme const& scheme{color_scheme};
    auto block = ColorScheme::blocks.at(glyph);
    std::stringstream output;
    auto [full, empty] = block;
    output << scheme.level_color(CommitNumberLevel::LEVEL0) << empty
           << scheme.reset << " 0 ";
    output << scheme.level_color(CommitNumberLevel::LEVEL1) << full
           << scheme.reset << " 1~2 ";
    output << scheme.level_color(CommitNumberLevel::LEVEL2) << full
           << scheme.reset << " 3~5 ";
    output << scheme.level_color(CommitNumberLevel::LEVEL3) << full
           << scheme.reset << " 6~10 ";
    output << scheme.level_color(CommitNumberLevel::LEVEL4) << full
           << scheme.reset << " >10";

    return output.str();
}

std::string Terminal::show_example2(std::string const& color_scheme,
                                    std::string const& glyph) {
    ColorScheme const& scheme{color_scheme};
    auto block = ColorScheme::blocks.at(glyph);
    std::stringstream output;
    auto [full, empty] = block;
    output << scheme.reset << " ";
    output << scheme.level_color(CommitNumberLevel::LEVEL0) << empty
           << scheme.reset << " ";
    output << scheme.level_color(CommitNumberLevel::LEVEL0) << empty
           << scheme.reset << " ";
    output << scheme.level_color(CommitNumberLevel::LEVEL1) << full
           << scheme.reset << " ";
    output << scheme.level_color(CommitNumberLevel::LEVEL2) << full
           << scheme.reset << " ";
    output << scheme.level_color(CommitNumberLevel::LEVEL3) << full
           << scheme.reset << " ";
    output << scheme.level_color(CommitNumberLevel::LEVEL4) << full
           << scheme.reset;

    return output.str();
}

#ifndef __GIT_HEATMAP_TERMINAL_H__
#define __GIT_HEATMAP_TERMINAL_H__

#include <array>
#include <chrono>
#include <map>
#include <string>

enum class CommitNumberLevel {
    LEVEL0 = 0, /* 0 */
    LEVEL1,     /* 1~2 */
    LEVEL2,     /* 3~5 */
    LEVEL3,     /* 6~8 */
    LEVEL4      /* 9~ */
};

class ColorScheme {
   public:
    using Scheme = std::array<const char*, 5>;

    ColorScheme(std::string const& color);
    static const std::map<std::string, Scheme> default_schemes;
    static const std::string info;
    static const std::string reset;
    static const std::map<std::string, std::pair<const char*, const char*>>
        blocks;
    std::string level_color(CommitNumberLevel level) const;

   private:
    Scheme current_color;
};

class Terminal {
   public:
    Terminal(std::string const& color_scheme, std::string const& glyph);
    int columns() const;
    std::string info_color() const;
    std::string reset_color() const;
    std::string level_color(CommitNumberLevel level) const;

    void display(
        std::vector<std::pair<const std::chrono::sys_days, int>> commits);

   private:
    ColorScheme color_scheme;
    std::pair<const char*, const char*> glyph;
};

#endif  // __GIT_HEATMAP_TERMINAL_H__

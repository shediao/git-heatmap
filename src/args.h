#ifndef __GIT_HEATMAP_ARGS_H__
#define __GIT_HEATMAP_ARGS_H__
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

#include "argparse/argparse.hpp"
#include "utils.h"

std::chrono::hours timezon_offset();
class Args;
Args& GetArgs();
class Args {
   public:
    Args();
    struct DateParser {
        std::string date_str_;
        explicit DateParser(std::string date_str) : date_str_(date_str) {}
        explicit operator std::chrono::sys_days() const {
            std::tm tm = {};
            std::istringstream ss(date_str_);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            if (ss.fail()) {
                throw std::runtime_error("Invalid date format. Use YYYY-MM-DD");
            }
            auto ret = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            auto hours_offset = timezon_offset();
            return std::chrono::floor<std::chrono::days>(ret + hours_offset);
        }
    };

    argparse::ArgParser parser_;
    std::string repo_path_{std::filesystem::current_path().string()};
    std::string email_pattern_{};
    std::string branch_{"HEAD"};
    std::string scheme_{"default"};
    std::string glyph_{"square"};
    int weeks_{MAX_DISPLAY_WEEKS};
    bool show_help_info_{false};
    bool debug_{false};
    void parse(int argc, const char* argv[]);
};

#endif  // __GIT_HEATMAP_ARGS_H__

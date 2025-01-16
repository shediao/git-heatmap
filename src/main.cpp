#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include "argparse.hpp"
#include "args.h"
#include "debug.h"
#include "git_analyzer.h"
#include "git_heatmap.h"
#include "time_utils.h"

#if defined(__clang__) && defined(_WIN32)
#include <windows.h>
static void __attribute__((__constructor__)) git_heatmap_configure_console_mode(
    void) {
    static UINT uiPrevConsoleCP = GetConsoleOutputCP();
    atexit([]() { SetConsoleOutputCP(uiPrevConsoleCP); });
    SetConsoleOutputCP(CP_UTF8);
}
#endif

int main(int argc, const char* argv[]) {
    try {
        Args& args = GetArgs();
        args.parse(argc, argv);

        DEBUG_LOG("Arguments parsed: repo_path="
                  << args.repo_path << ", author_email=" << args.author_email
                  << ", branch=" << args.branch);

        if (args.show_help_info) {
            DEBUG_LOG("Showing help information");
            args.parser.print_usage();
            return 0;
        }

        if (!std::filesystem::exists(args.repo_path)) {
            DEBUG_LOG("Repository path does not exist: " << args.repo_path);
            std::cerr << "Error: repo path does not exist" << std::endl;
            return 1;
        }

#ifdef min
#undef min
#endif
        size_t display_weeks =
            std::min(HeatMap::max_display_weeks(), args.weeks);
        DEBUG_LOG("display weeks: " << display_weeks);
        DEBUG_LOG("now: " << std::format("{:%Y-%m-%d %H:%M:%S}", now()));

        DEBUG_LOG("Initializing GitAnalyzer");
        GitAnalyzer analyzer(args.repo_path);

        // 今天的时间， 精确到天
        auto end_time = std::chrono::floor<std::chrono::days>(now());
        auto start_time = end_time - std::chrono::days(display_weeks * 7);
        DEBUG_LOG("Analyzing commits from "
                  << std::format("{:%Y-%m-%d %H:%M:%S}", start_time) << " to "
                  << std::format("{:%Y-%m-%d %H:%M:%S}", end_time));

        auto analysis = analyzer.analyze_commits(args.author_email, args.branch,
                                                 start_time, end_time);

        DEBUG_LOG("Creating commit calendar");
        HeatMap calendar(analysis.commit_counts, args.author_email, args.scheme,
                         analysis.start_date, analysis.end_date);

        DEBUG_LOG("Displaying calendar");
        calendar.display();

    } catch (const std::exception& e) {
        DEBUG_LOG("Error occurred: " << e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

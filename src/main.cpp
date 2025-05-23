#include <algorithm>
#include <filesystem>
#include <iostream>

#include "argparse/argparse.hpp"
#include "args.h"
#include "debug.h"
#include "heatmap.h"
#include "utils.h"

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
                  << args.repo_path << ", email_pattern=" << args.email_pattern
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

        auto weeks = std::clamp(args.weeks, 4, MAX_DISPLAY_WEEKS);
        auto end_days = sunday();
        auto start_days = end_days - std::chrono::days(weeks * 7 - 1);
        GitHeatMap heatmap(args.repo_path, args.branch, args.email_pattern,
                           args.scheme, args.glyph, start_days, end_days);

        heatmap.display();

    } catch (const std::exception& e) {
        DEBUG_LOG("Error occurred: " << e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

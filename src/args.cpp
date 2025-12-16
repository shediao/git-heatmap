
#include "args.h"

#include "argparse/argparse.hpp"
#include "glob.h"
#include "terminal.h"

// TODO:
// --days
// --since
// --until
// --include-merges
// --no-merges

Args::Args() : parser_("git-heatmap", "Git Contribution Heatmap") {
    parser_.add_flag("h,help", "show help info", this->show_help_info_);
    parser_.add_option("repo", "git repository path", this->repo_path_)
        .value_help("dir");
    parser_
        .add_option("a,author", "author email pattern(default: user's email)",
                    this->email_pattern_)
        .value_help("pattern");
    parser_
        .add_option("e,email", "author email pattern(default: user's email)",
                    this->email_pattern_)
        .value_help("pattern")
        .hidden();
    parser_.add_option("b,branch", "branch name", this->branch_)
        .default_value("HEAD");
    parser_.add_option("scheme", "color scheme", this->scheme_)
        .default_value("default")
        .choices([](auto const& scheme) {
            std::vector<std::string> ret;
            for (auto& [key, value] : scheme) {
                ret.push_back(key);
            }
            return ret;
        }(ColorScheme::default_schemes))
        .choices_description([](auto const& scheme) {
            std::map<std::string, std::string> ret;
            for (auto& [key, value] : scheme) {
                ret[key] = Terminal::show_example(key, "square");
            }
            return ret;
        }(ColorScheme::default_schemes));
    parser_
        .add_option("glyph", "heatmap glyph", this->glyph_)
#ifdef _WIN32
        .default_value("fisheye")
#else
        .default_value("square")
#endif
        .choices([](auto const& blocks) {
            std::vector<std::string> ret;
            for (auto& [key, block] : blocks) {
                ret.push_back(key);
            }
            return ret;
        }(ColorScheme::blocks))
        .choices_description(
            [](std::map<std::string, std::pair<const char*, const char*>> const&
                   blocks) {
                std::map<std::string, std::string> ret;
                for (auto& [key, block] : blocks) {
                    ret[key] = Terminal::show_example2("default", key);
                }
                return ret;
            }(ColorScheme::blocks));

    parser_.add_flag("d,debug", "enable debug mode", this->debug_).hidden();
    parser_.add_positional("repository", "alias of --repo", this->repo_path_);
}
void Args::parse(int argc, const char* argv[]) {
    parser_.parse(argc, argv);

    if (!this->email_pattern_.empty() &&
        !is_valid_glob_pattern(this->email_pattern_)) {
        throw std::invalid_argument("Invalid email pattern: " +
                                    this->email_pattern_);
    }
}

Args& GetArgs() {
    static Args args;
    return args;
}

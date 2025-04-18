
#include "args.h"

#include "argparse.hpp"
#include "glob.h"

// TODO:
// --days
// --since
// --until
// --include-merges
// --no-merges

Args::Args() : parser("git-heatmap", "Git Contribution Heatmap") {
    parser.add_flag("h,help", "show help info", this->show_help_info);
    parser.add_option("repo", "git repository path", this->repo_path);
    parser.add_option(
        "a,author",
        "author email pattern(default: git config --get user.email)",
        this->author_email);
    parser.add_option(
        "e,email", "author email pattern(default: git config --get user.email)",
        this->author_email);
    parser.add_option("b,branch", "branch name", this->branch)
        .default_value("HEAD");
    parser.add_option("scheme", "color scheme", this->scheme)
        .default_value("default")
        .choices({"default", "dracula", "vibrant"});
    parser
        .add_option("glyph", "heatmap glyph", this->glyph)
#ifdef _WIN32
        .default_value("fisheye")
#else
        .default_value("square")
#endif
        .choices({"block", "square", "dot", "fisheye", "diamond", "plus"});

    parser.add_flag("d,debug", "enable debug mode", this->debug);
    parser.add_positional("repository", "alias of --repo", repo_path);
}
void Args::parse(int argc, const char* argv[]) {
    parser.parse(argc, argv);

    if (!this->author_email.empty() &&
        !is_valid_glob_pattern(this->author_email)) {
        throw std::invalid_argument("Invalid email pattern: " +
                                    this->author_email);
    }
}

Args& GetArgs() {
    static Args args;
    return args;
}

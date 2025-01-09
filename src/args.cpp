
#include "args.h"

#include "argparse.hpp"
#include "glob.h"

Args::Args()
    : parser("git-heatmap", "Git Contribution Heatmap (Git 提交贡献热力图)") {
    parser.add_flag("h,help", "show help info", this->show_help_info);
    parser.add_option("repo", "git repository path", this->repo_path);
    parser.add_option("email",
                      "author email(default: git config --get user.email)",
                      this->author_email);
    parser.add_option("b,branch", "branch name", this->branch)
        .default_value("HEAD");
    parser.add_option("w,weeks", "the number of display weeks", this->weeks);
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

    if (this->author_email.empty()) {
        // this->author_email = get_git_config("user.email");
    } else {
        if (!is_valid_glob_pattern(this->author_email)) {
            throw std::invalid_argument("Invalid email pattern: " +
                                        this->author_email);
        }
    }
}

Args& GetArgs() {
    static Args args;
    return args;
}

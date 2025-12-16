#include "heatmap.h"

#include <cassert>
#include <chrono>
#include <memory>

#include "git2/repository.h"
#include "git2/types.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <git2.h>

#include "debug.h"
#include "glob.h"
#include "terminal.h"
#include "utils.h"

using git_repository_ptr =
    std::unique_ptr<git_repository, decltype([](git_repository* repo) {
                        git_repository_free(repo);
                    })>;

using git_config_ptr =
    std::unique_ptr<git_config, decltype([](git_config* config) {
                        git_config_free(config);
                    })>;

using git_reference_ptr =
    std::unique_ptr<git_reference, decltype([](git_reference* ref) {
                        git_reference_free(ref);
                    })>;
using git_revwalk_ptr =
    std::unique_ptr<git_revwalk, decltype([](git_revwalk* walk) {
                        git_revwalk_free(walk);
                    })>;

using git_commit_ptr =
    std::unique_ptr<git_commit, decltype([](git_commit* commit) {
                        git_commit_free(commit);
                    })>;

using git_buf_ptr =
    std::unique_ptr<git_buf, decltype([](git_buf* buf) { git_buf_free(buf); })>;

constexpr static int MAX_CHECK_COUNT = 100;

class GitHeatMap::HeatMapImpl {
    class EmailMatcher {
       public:
        EmailMatcher(std::string const& email) : email_(email) {
            set_pattern(email);
        }
        bool operator()(std::string const& email) {
            if (email_.empty()) {
                return true;
            }
            if (is_pattern_) {
                return matchglob(email_, email);
            }
            return email.find(email_) != std::string::npos;
        }

        void set_pattern(std::string const& email) {
            email_ = email;
            is_pattern_ = std::any_of(email.begin(), email.end(), [](char c) {
                return c == '?' || c == '*';
            });
        }

       private:
        std::string email_;
        bool is_pattern_{false};
    };

   public:
    HeatMapImpl(std::string repo_path, std::string branch,
                std::string email_pattern, std::string const& color_scheme,
                std::string const& glyph, std::chrono::sys_days start_days,
                std::chrono::sys_days end_days);
    ~HeatMapImpl() {
        if (repo_) {
            git_repository_free(repo_);
            repo_ = nullptr;
        }
    }
    void display();

   private:
    void get_branch_head(const std::string& branch_name, git_oid* oid);

   private:
    git_repository* repo_{nullptr};
    std::chrono::sys_days start_days_{std::chrono::days::zero()};
    std::chrono::sys_days end_days_{std::chrono::days::zero()};
    std::vector<std::pair<const std::chrono::sys_days, int>> commits_;
    EmailMatcher email_matcher_;
    Terminal terminal_;
};

static void ensure_libgit_init() {
    static bool initialized = false;
    if (!initialized) {
        git_libgit2_init();
        initialized = true;
    }
}
void GitHeatMap::HeatMapImpl::get_branch_head(const std::string& branch_name,
                                              git_oid* oid) {
    std::vector<std::string> refnames{};
    if (branch_name.starts_with("refs/")) {
        refnames.push_back(branch_name);
    } else {
        refnames = {branch_name, "refs/tags/" + branch_name,
                    "refs/heads/" + branch_name, "refs/remotes/" + branch_name,
                    "refs/remotes/origin/" + branch_name};
    }
    for (auto const& refname : refnames) {
        git_reference_ptr ref = [](git_repository* repo, const char* refname_) {
            git_reference* r;
            if (0 == git_reference_lookup(&r, repo, refname_)) {
                return git_reference_ptr(r);
            }
            return git_reference_ptr(nullptr);
        }(repo_, refname.c_str());
        if (ref) {
            if (git_reference_type(ref.get()) == GIT_REFERENCE_SYMBOLIC) {
                auto target_reference = [](git_reference* refname_) {
                    git_reference* target;
                    if (0 == git_reference_resolve(&target, refname_)) {
                        return git_reference_ptr(target);
                    }
                    return git_reference_ptr(nullptr);
                }(ref.get());
                git_oid_cpy(oid, git_reference_target(target_reference.get()));
            } else {
                git_oid_cpy(oid, git_reference_target(ref.get()));
            }
            return;
        }
    }
    throw std::runtime_error("Branch not found: " + branch_name);
}

GitHeatMap::HeatMapImpl::HeatMapImpl(std::string repo_path, std::string branch,
                                     std::string email_pattern,
                                     std::string const& color_scheme,
                                     std::string const& glyph,
                                     std::chrono::sys_days start_days,
                                     std::chrono::sys_days end_days)
    : start_days_{start_days},
      end_days_{end_days},
      email_matcher_{email_pattern},
      terminal_{color_scheme, glyph, email_pattern} {
    DEBUG_LOG("Initializing GitAnalyzer with repo path: " << repo_path);
    ensure_libgit_init();

    if (git_repository_open_ext(&repo_, repo_path.c_str(), 0, nullptr) != 0) {
        throw std::runtime_error("Failed to open repository");
    }

    DEBUG_LOG("today: " << today());
    DEBUG_LOG("monday: " << monday());
    DEBUG_LOG("sunday: " << sunday());
    DEBUG_LOG("start date: " << start_days_);
    DEBUG_LOG("end date: " << end_days_);
    DEBUG_LOG("branch: " << branch);

    for (auto i = start_days_; i <= end_days_; i = i + std::chrono::days(1)) {
        commits_.push_back({i, 0});
    }

    assert((commits_.size() % 7) == 0);

    if (email_pattern.empty()) {
        git_config_ptr config = [](git_repository* repo) {
            git_config* c{nullptr};
            if (0 == git_repository_config_snapshot(&c, repo)) {
                return git_config_ptr(c);
            }
            return git_config_ptr(nullptr);
        }(repo_);
        if (config) {
            const char* user_email = nullptr;
            if (0 == git_config_get_string(&user_email, config.get(),
                                           "user.email") &&
                nullptr != user_email) {
                email_pattern = user_email;
            } else {
                DEBUG_LOG(git_error_last()->message);
            }
        } else {
            DEBUG_LOG("git config get error.");
        }
    }
    if (!email_pattern.empty()) {
        email_matcher_.set_pattern(email_pattern);
        terminal_.set_author(email_pattern);
    }
    DEBUG_LOG("author: " << email_pattern);

    git_oid head_oid;
    get_branch_head(branch, &head_oid);
    git_commit_ptr head_commit = [](git_repository* repo, git_oid* oid) {
        git_commit* c;
        if (0 == git_commit_lookup(&c, repo, oid)) {
            return git_commit_ptr(c);
        }
        return git_commit_ptr(nullptr);
    }(repo_, &head_oid);

    git_revwalk_ptr walk = [](git_repository* repo) {
        git_revwalk* w{nullptr};
        if (0 == git_revwalk_new(&w, repo)) {
            return git_revwalk_ptr(w);
        }
        return git_revwalk_ptr(nullptr);
    }(repo_);

    if (!walk) {
        throw std::runtime_error("Failed to create git walk");
    }

    git_revwalk_push(walk.get(), git_commit_id(head_commit.get()));
    git_revwalk_sorting(walk.get(), GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);

    git_oid oid;
    int check_count = 0;

    while (0 == git_revwalk_next(&oid, walk.get())) {
        git_commit_ptr commit = [](git_repository* repo, git_oid* o) {
            git_commit* c;
            if (0 == git_commit_lookup(&c, repo, o)) {
                return git_commit_ptr(c);
            }
            return git_commit_ptr(nullptr);
        }(repo_, &oid);
        if (!commit) {
            break;
        }
        auto commit_days = std::chrono::floor<std::chrono::days>(
            std::chrono::system_clock::from_time_t(
                git_commit_time(commit.get())) +
            timezon_offset());
        if (commit_days < start_days_) {
            if (check_count++ > MAX_CHECK_COUNT) {
                break;
            }
        } else {
            check_count = 0;
        }
        auto const* author = git_commit_author(commit.get());
        auto email = std::string(author->email);

        char sha1[GIT_OID_HEXSZ + 1] = {0};
        git_oid_fmt(sha1, &oid);
        sha1[GIT_OID_HEXSZ] = '\0';

        if (commit_days >= start_days_ && commit_days <= end_days_ &&
            email_matcher_(email)) {
            commits_[(commit_days - start_days_).count()].second++;
        } else {
            DEBUG_LOG("Skipping commit at time: "
                      << std::format("{:%Y-%m-%d}",
                                     std::chrono::year_month_day{commit_days})
                      << " by " << email << " sha1: " << sha1);
        }
    }
}
void GitHeatMap::HeatMapImpl::display() { terminal_.display(commits_); }

GitHeatMap::GitHeatMap(std::string repo_path, std::string branch,
                       std::string email_pattern,
                       std::string const& color_scheme,
                       std::string const& glyph,
                       std::chrono::sys_days start_days,
                       std::chrono::sys_days end_days)
    : impl(std::make_unique<HeatMapImpl>(
          std::move(repo_path), std::move(branch), std::move(email_pattern),
          color_scheme, glyph, start_days, end_days)) {}

GitHeatMap::~GitHeatMap() {}

void GitHeatMap::display() { impl->display(); }

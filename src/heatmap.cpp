#include "heatmap.h"

#include <cassert>
#include <chrono>
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

template <typename T, typename Del>
class GitResourceGuard {
   public:
    GitResourceGuard(T* ref, Del del) : ref(ref), del(del) {}
    ~GitResourceGuard() {
        if (ref) {
            del(ref);
        }
    }
    T** operator&() { return &ref; }
    T* get() const { return ref; }

   private:
    T* ref;
    Del del;
};

constexpr static int MAX_CHECK_COUNT = 100;

class GitHeatMap::HeatMapImpl {
   public:
    HeatMapImpl(std::string repo_path, std::string branch, std::string email,
                std::string const& color_scheme, std::string const& glyph,
                std::chrono::sys_days start_days,
                std::chrono::sys_days end_days);
    ~HeatMapImpl() {
        if (repo) {
            git_repository_free(repo);
            repo = nullptr;
        }
    }
    void display();

   private:
    git_oid get_branch_head(const std::string& branch_name);

   private:
    git_repository* repo{nullptr};
    std::chrono::sys_days start_days{std::chrono::days::zero()};
    std::chrono::sys_days end_days{std::chrono::days::zero()};
    std::vector<std::pair<const std::chrono::sys_days, int>> commits;
    Terminal terminal;
};

static void ensure_libgit_init() {
    static bool initialized = false;
    if (!initialized) {
        git_libgit2_init();
        initialized = true;
    }
}
git_oid GitHeatMap::HeatMapImpl::get_branch_head(
    const std::string& branch_name) {
    GitResourceGuard<git_reference, decltype(&git_reference_free)> ref(
        nullptr, &git_reference_free);
    std::vector<std::string> refnames{};
    if (branch_name.starts_with("refs/")) {
        refnames.push_back(branch_name);
    } else {
        refnames = {branch_name, "refs/tags/" + branch_name,
                    "refs/heads/" + branch_name, "refs/remotes/" + branch_name,
                    "refs/remotes/origin/" + branch_name};
    }
    for (auto const& refname : refnames) {
        if (0 == git_reference_lookup(&ref, repo, refname.c_str())) {
            git_oid oid;
            if (git_reference_type(ref.get()) == GIT_REFERENCE_SYMBOLIC) {
                GitResourceGuard<git_reference, decltype(&git_reference_free)>
                    target_reference(nullptr, &git_reference_free);
                git_reference_resolve(&target_reference, ref.get());
                git_oid_cpy(&oid, git_reference_target(target_reference.get()));
            } else {
                git_oid_cpy(&oid, git_reference_target(ref.get()));
            }
            return oid;
        }
    }
    throw std::runtime_error("Branch not found: " + branch_name);
}

GitHeatMap::HeatMapImpl::HeatMapImpl(std::string repo_path, std::string branch,
                                     std::string email,
                                     std::string const& color_scheme,
                                     std::string const& glyph,
                                     std::chrono::sys_days start_days,
                                     std::chrono::sys_days end_days)
    : start_days{start_days},
      end_days{end_days},
      terminal{color_scheme, glyph} {
    DEBUG_LOG("Initializing GitAnalyzer with repo path: " << repo_path);
    ensure_libgit_init();

    git_buf buf = GIT_BUF_INIT;
    GitResourceGuard<git_buf, decltype(&git_buf_free)> buf_guard(&buf,
                                                                 &git_buf_free);
    if (git_repository_discover(buf_guard.get(), repo_path.c_str(), 0,
                                nullptr) != 0) {
        throw std::runtime_error("Failed to discover repository");
    }
    if (git_repository_open(&repo, buf_guard.get()->ptr) != 0) {
        throw std::runtime_error("Failed to open repository");
    }

    DEBUG_LOG("today: " << today());
    DEBUG_LOG("monday: " << monday());
    DEBUG_LOG("sunday: " << sunday());
    DEBUG_LOG("start date: " << start_days);
    DEBUG_LOG("end date: " << end_days);
    DEBUG_LOG("branch: " << branch);

    for (auto i = start_days; i <= end_days; i = i + std::chrono::days(1)) {
        commits.push_back({i, 0});
    }

    DEBUG_LOG("commits number: " << commits.size());

    assert((commits.size() % 7) == 0);

    if (email.empty()) {
        GitResourceGuard<git_config, decltype(&git_config_free)> config_guard{
            nullptr, &git_config_free};
        if (git_repository_config_snapshot(&config_guard, repo) == 0) {
            const char* user_email = nullptr;
            if (0 == git_config_get_string(&user_email, config_guard.get(),
                                           "user.email") &&
                nullptr != user_email) {
                email = user_email;
            }
        }
    }

    DEBUG_LOG("author: " << email);

    git_oid branch_head = get_branch_head(branch);

    git_commit* branch_head_commit;
    git_commit_lookup(&branch_head_commit, repo, &branch_head);

    git_commit* commit = branch_head_commit;

    int check_count = 0;
    do {
        if (!commit) {
            break;
        }
        GitResourceGuard<git_commit, decltype(&git_commit_free)> commit_guard(
            commit, &git_commit_free);

        // TODO: get all parent commits
        commit = nullptr;
        git_commit_parent(&commit, commit_guard.get(), 0);

        auto commit_days = std::chrono::floor<std::chrono::days>(
            std::chrono::system_clock::from_time_t(
                git_commit_time(commit_guard.get())) +
            timezon_offset());
        if (commit_days < start_days) {
            if (check_count++ > MAX_CHECK_COUNT) {
                break;
            }
        } else {
            check_count = 0;
        }
        auto const* author = git_commit_author(commit_guard.get());
        auto email = std::string(author->email);

        char sha1[GIT_OID_HEXSZ + 1] = {0};
        const git_oid* oid = git_commit_id(commit_guard.get());
        git_oid_fmt(sha1, oid);
        sha1[GIT_OID_HEXSZ] = '\0';

        if (commit_days >= start_days && commit_days <= end_days &&
            (email.empty() || matchglob(email, email))) {
            commits[(commit_days - start_days).count()].second++;
        } else {
            DEBUG_LOG("Skipping commit at time: "
                      << std::format("{:%Y-%m-%d}",
                                     std::chrono::year_month_day{commit_days})
                      << " by " << email << " sha1: " << sha1);
        }
    } while (commit != nullptr);
}
void GitHeatMap::HeatMapImpl::display() { terminal.display(commits); }

GitHeatMap::GitHeatMap(std::string repo_path, std::string branch,
                       std::string email, std::string const& color_scheme,
                       std::string const& glyph,
                       std::chrono::sys_days start_days,
                       std::chrono::sys_days end_days)
    : impl(std::make_unique<HeatMapImpl>(
          std::move(repo_path), std::move(branch), std::move(email),
          color_scheme, glyph, start_days, end_days)) {}

GitHeatMap::~GitHeatMap() {}

void GitHeatMap::display() { impl->display(); }

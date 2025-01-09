#include "git_analyzer.h"

#include <chrono>
#include <ctime>
#include <stdexcept>

#include "debug.h"
#include "git_utils.h"
#include "glob.h"

std::chrono::hours timezon_offset();
namespace {}

GitAnalyzer::GitAnalyzer(const std::string& repo_path) {
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
}

GitAnalyzer::~GitAnalyzer() { git_repository_free(repo); }

void GitAnalyzer::ensure_libgit_init() {
    static bool initialized = false;
    if (!initialized) {
        git_libgit2_init();
        initialized = true;
    }
}

git_oid GitAnalyzer::get_branch_head(const std::string& branch_name) {
    DEBUG_LOG("Getting branch head for: " << branch_name);
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

CommitAnalysis GitAnalyzer::analyze_commits(
    std::string& author_email_pattern, const std::string& branch,
    const std::chrono::sys_days& start_days,
    const std::chrono::sys_days& end_days) {
    if (author_email_pattern.empty()) {
        GitResourceGuard<git_config, decltype(&git_config_free)> config_guard{
            nullptr, &git_config_free};
        if (git_repository_config_snapshot(&config_guard, repo) == 0) {
            const char* user_email = nullptr;
            if (0 == git_config_get_string(&user_email, config_guard.get(),
                                           "user.email") &&
                nullptr != user_email) {
                author_email_pattern = user_email;
            }
        }
    }
    DEBUG_LOG("Analyzing commits for author: " << author_email_pattern
                                               << ", branch: " << branch);
    DEBUG_LOG("Start time: " << std::format("{:%Y-%m-%d}", start_days));
    DEBUG_LOG("End time: " << std::format("{:%Y-%m-%d}", end_days));

    CommitAnalysis result;
    git_oid branch_head = get_branch_head(branch);

    constexpr int MAX_CHECK_COUNT = 100;

    git_commit* branch_head_commit;
    git_commit_lookup(&branch_head_commit, repo, &branch_head);

    git_commit* commit = branch_head_commit;

    int check_count = 0;
    do {
        if (!commit) {
            DEBUG_LOG("No more commits to analyze");
            break;
        }
        GitResourceGuard<git_commit, decltype(&git_commit_free)> commit_guard(
            commit, &git_commit_free);
        commit = nullptr;
        git_commit_parent(&commit, commit_guard.get(), 0);

        auto commit_time = std::chrono::floor<std::chrono::days>(
            std::chrono::system_clock::from_time_t(
                git_commit_time(commit_guard.get())) +
            timezon_offset());
        if (commit_time < start_days) {
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

        if (commit_time >= start_days && commit_time <= end_days &&
            (author_email_pattern.empty() ||
             matchglob(author_email_pattern, email))) {
            DEBUG_LOG("Found matching commit at time: "
                      << std::format("{:%Y-%m-%d %H:%M:%S}", commit_time)
                      << " by " << email << " sha1: " << sha1);
            result.commit_counts[commit_time]++;
        } else {
            DEBUG_LOG("Skipping commit at time: "
                      << std::format("{:%Y-%m-%d %H:%M:%S}", commit_time)
                      << " by " << email << " sha1: " << sha1);
        }
    } while (commit != nullptr);

    result.end_date = end_days;
    result.start_date = start_days;

    return result;
}

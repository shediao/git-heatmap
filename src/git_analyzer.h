#ifndef __GIT_HEATMAP_GIT_ANALYZER_H__
#define __GIT_HEATMAP_GIT_ANALYZER_H__
#pragma once
#include <git2.h>

#include <chrono>
#include <map>
#include <string>

struct CommitAnalysis {
    std::map<std::chrono::sys_days, int> commit_counts;
    std::chrono::sys_days start_date;
    std::chrono::sys_days end_date;
};

class GitAnalyzer {
   public:
    explicit GitAnalyzer(const std::string& repo_path);
    ~GitAnalyzer();

    CommitAnalysis analyze_commits(std::string& author_email_pattern,
                                   const std::string& branch,
                                   const std::chrono::sys_days& start_time,
                                   const std::chrono::sys_days& end_time);

   private:
    git_repository* repo;
    void ensure_libgit_init();
    git_oid get_branch_head(const std::string& branch_name);
};

#endif  // __GIT_HEATMAP_GIT_ANALYZER_H__

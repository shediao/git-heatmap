#ifndef __GIT_HEATMAP_H__
#define __GIT_HEATMAP_H__

#include <chrono>
#include <memory>
#include <string>

class GitHeatMap {
   public:
    GitHeatMap(std::string repo_path, std::string branch, std::string email,
               std::string const& color_scheme, std::string const& glyph,
               std::chrono::sys_days start_days,
               std::chrono::sys_days end_days);
    ~GitHeatMap();
    void display();

   private:
    class HeatMapImpl;
    std::unique_ptr<HeatMapImpl> impl;
};

#endif  // __GIT_HEATMAP_COMMIT_CALENDAR_H__

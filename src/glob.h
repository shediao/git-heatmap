
#ifndef __GIT_HEATMAP_GLOB_H__
#define __GIT_HEATMAP_GLOB_H__

#include <string>
#include <vector>
bool is_valid_glob_pattern(const std::string& pattern);

bool matchglob(const std::string& pattern, const std::string& name);

bool matchglobs(const std::vector<std::string>& patterns,
                const std::string& name);

#endif  // __GIT_HEATMAP_GLOB_H__


#include "glob.h"

#include <algorithm>
#include <stack>

bool is_valid_glob_pattern(const std::string& pattern) {
    for (auto i = pattern.cbegin(); i != pattern.cend(); ++i) {
        if (*i == '*' || *i == '?') {
            const auto j = i + 1;
            if (j != pattern.cend() && (*j == '*' || *j == '?')) {
                return false;
            }
        }
    }
    return true;
}

bool matchglob(const std::string& pattern, const std::string& name) {
    const char* p = pattern.c_str();
    const char* n = name.c_str();
    std::stack<std::pair<const char*, const char*>,
               std::vector<std::pair<const char*, const char*>>>
        backtrack;

    for (;;) {
        bool matching = true;
        while (*p != '\0' && matching) {
            switch (*p) {
                case '*':
                    // Step forward until we match the next character after *
                    while (*n != '\0' && *n != p[1]) {
                        n++;
                    }
                    if (*n != '\0') {
                        // If this isn't the last possibility, save it for later
                        backtrack.emplace(p, n);
                    }
                    break;
                case '?':
                    // Any character matches unless we're at the end of the name
                    if (*n != '\0') {
                        n++;
                    } else {
                        matching = false;
                    }
                    break;
                default:
                    // Non-wildcard characters match literally
                    if (*n == *p) {
                        n++;
                    } else if (*n == '\\' && *p == '/') {
                        n++;
                    } else if (*n == '/' && *p == '\\') {
                        n++;
                    } else {
                        matching = false;
                    }
                    break;
            }
            p++;
        }

        // If we haven't failed matching and we've reached the end of the name,
        // then success
        if (matching && *n == '\0') {
            return true;
        }

        // If there are no other paths to try, then fail
        if (backtrack.empty()) {
            return false;
        }

        // Restore pointers from backtrack stack
        p = backtrack.top().first;
        n = backtrack.top().second;
        backtrack.pop();

        // Advance name pointer by one because the current position didn't work
        n++;
    }
}

bool matchglobs(const std::vector<std::string>& patterns,
                const std::string& name) {
    return std::any_of(begin(patterns), end(patterns),
                       [&name](const std::string& pattern) {
                           return matchglob(pattern, name);
                       });
}

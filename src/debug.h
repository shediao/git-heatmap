#ifndef __GIT_HEATMAP_DEBUG_H__
#define __GIT_HEATMAP_DEBUG_H__

#include <iostream>

#include "args.h"

#define DEBUG_LOG(msg)                                   \
    do {                                                 \
        if (GetArgs().debug) {                           \
            std::cerr << "[DEBUG] " << msg << std::endl; \
        }                                                \
    } while (0)

#endif  // __GIT_HEATMAP_DEBUG_H__

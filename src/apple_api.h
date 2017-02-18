#ifndef APPLE_API_
#define APPLE_API_

#include "WindowList.h"
#include "DisplayList.h"

namespace apple_api
{

extern
    bool enable_accessibility_api(); // true if enabled
extern
    std::pair<WindowList, DisplayList> build_lists(bool front_only = false);
extern
    bool apply_bounds(const Window &window, Bounds bounds); // true if success

}

#endif /* APPLE_API_ */
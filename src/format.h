#ifndef FORMAT_H_
#define FORMAT_H_

#include "Bounds.h"

#include <iostream>
#include <iomanip>

inline std::ostream& operator<<(std::ostream &os, const Bounds &b)
{
    return os << "pos("   << std::setw(5) << b.x << ","
                          << std::setw(5) << b.y << ")"
                 " size(" << std::setw(5) << b.w << ","
                          << std::setw(5) << b.h << ")";
}

struct fmt_window {
    const Window *window;
    fmt_window(const Window &window) : window(&window) {}
};

// 24 + 5 + 50 = 79
inline std::ostream& operator<<(std::ostream &os, const fmt_window &in) {
    return os << std::setw(24) << in.window->owner.substr(0, 24)
        << " " << in.window->index << " : "
        << std::left << std::setw(50) << in.window->title.substr(0, 50)
        << std::right << '\n'
        << std::setw(24) << "" << "     " << in.window->bounds << '\n' ;
}

#endif /* FORMAT_H_ */

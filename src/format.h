#ifndef FORMAT_H_
#define FORMAT_H_

#include "Bounds.h"

#include <iostream>
#include <iomanip>

inline std::ostream& operator<<(std::ostream &os, const Bounds &b) {
    return os << "origin(" << std::setw(5) << b.x << ","
                           << std::setw(5) << b.y << ")"
                 " size("  << std::setw(5) << b.w << ","
                           << std::setw(5) << b.h << ")";
}


// formatting helper classes
#define FMT_DEFINE(name_, w_, vtype_, itype_, isuffix_) \
    struct name_ { \
        constexpr static int w = w_; \
        vtype_ val; \
        name_ (itype_ in) : val(in isuffix_) {} \
    };

FMT_DEFINE(fmt_owner, 24, STR, CSR, );
FMT_DEFINE(fmt_title, 50, STR, CSR, );

#undef FMT_DEFINE

// 24 + 5 + 50 = 79
inline std::ostream& operator<<(std::ostream &os, const fmt_owner &in) {
    return os << std::setw(fmt_owner::w)
              << in.val.substr(0, fmt_owner::w);
}

inline std::ostream& operator<<(std::ostream &os, const fmt_title &in) {
    return os << std::left << std::setw(fmt_title::w)
              << in.val.substr(0, fmt_title::w) << std::right;
}


struct fmt_window {
    STR owner;
    std::size_t index;
    Window window;
    fmt_window(CSR owner, std::size_t index, const Window &window)
        : owner(owner), index(index), window(window) {}
};

inline std::ostream& operator<<(std::ostream &os, const fmt_window &in) {
    return os << fmt_owner(in.owner) << " " << in.index << " : "
                << fmt_title(in.window.title) << '\n'
                << std::setw(fmt_owner::w) << ""
                << "     " << in.window.bounds << '\n' ;
}

#endif /* FORMAT_H_ */

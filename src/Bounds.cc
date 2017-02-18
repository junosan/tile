#include "Bounds.h"

#include <algorithm>
#include <cmath>

int Bounds::overlap_area(Bounds other) const
{
    auto overlap_len = [](int o1, int s1, int o2, int s2) {
        return (o1 < o2 ? std::min(std::max(o1 + s1 - o2, 0), s2) :
                            std::min(std::max(o2 + s2 - o1, 0), s1));
    };
    return overlap_len(x, w, other.x, other.w) *
            overlap_len(y, h, other.y, other.h);
}

// static
std::pair<int, int>
    Bounds::sub(int origin, int size, float unit, float begin, float end)
{
    int rel_orig = std::min(static_cast<int>(std::round(unit * begin)),
                            size);
    int rel_term = std::min(static_cast<int>(std::round(unit * end)),
                            size);
    rel_orig = std::min(rel_orig, rel_term - 1);

    return std::make_pair(origin + rel_orig, rel_term - rel_orig);
}

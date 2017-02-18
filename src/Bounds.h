#ifndef BOUNDS_H_
#define BOUNDS_H_

#include "common.h"

#include <tuple>

struct Bounds
{
    int x, y, w, h; // use aggregate initialization

    int overlap_area(Bounds other) const;
    static std::pair<int, int> // sub'd (origin, size)
        sub(int origin, int size, float unit, float begin, float end);
};

struct Window
{
    Bounds bounds;
    STR title;
    unsigned int pid;
};

#endif /* BOUNDS_H_ */

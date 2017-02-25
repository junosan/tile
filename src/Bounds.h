#ifndef BOUNDS_H_
#define BOUNDS_H_

#include "common.h"
#include "ArgParser.h"

#include <tuple>

struct Bounds
{
    int x, y, w, h; // use aggregate initialization

    int overlap_area(Bounds other) const;
    void attach(Bounds other); // bounding box encloses both other and *this

    enum class snap_dir { l, r, t, b };

    // return in with bounds overwritten
    Bounds snap (Bounds in, snap_dir dir) const;
    Bounds fit  (Bounds in) const;
    Bounds h_sub(Bounds in, float unit, float l, float r) const;
    Bounds v_sub(Bounds in, ArgParser::v_cmd v_cmd) const;
};

struct Window
{
    Bounds bounds;
    STR owner;
    STR title;
    std::size_t index;
    int pid;
};

#endif /* BOUNDS_H_ */

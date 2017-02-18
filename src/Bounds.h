#ifndef BOUNDS_H_
#define BOUNDS_H_

#include "common.h"
#include "ArgParser.h"

#include <tuple>

struct Bounds
{
    int x, y, w, h; // use aggregate initialization

    int overlap_area(Bounds other) const;

    // return in with sub'd bounds overwritten
    Bounds h_sub(Bounds in, float unit, float l, float r);
    Bounds v_sub(Bounds in, ArgParser::v_cmd v_cmd);
    Bounds fit  (Bounds in);

};

struct Window
{
    Bounds bounds;
    STR title;
    unsigned int pid;
};

#endif /* BOUNDS_H_ */

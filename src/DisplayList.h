#ifndef DISPLAYLIST_H_
#define DISPLAYLIST_H_

#include "Bounds.h"

#include <vector>

class DisplayList
{
public:
    DisplayList() : margins{0, 0, 0, 0} {}

    void set_margins(std::tuple<int, int, int, int> margins);

    void add_display(Bounds display);

    // find display based on most overlap
    // return display (+ idx_offset) after margins applied
    Bounds target_display(Bounds window, std::ptrdiff_t idx_offset = 0);

    const std::vector<Bounds>& get_vec();

private:
    //          l    r    t    b
    std::tuple<int, int, int, int> margins;
    std::vector<Bounds> displays; // before margins applied
    std::vector<int>    overlaps;
};

#endif /* DISPLAYLIST_H_ */

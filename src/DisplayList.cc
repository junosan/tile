#include "DisplayList.h"

void DisplayList::set_margins(std::tuple<int, int, int, int> margins)
{
    this->margins = margins;
}

void DisplayList::add_display(Bounds display)
{
    displays.emplace_back(display);
}

Bounds DisplayList::target_display(Bounds window, std::ptrdiff_t idx_offset)
{
    std::ptrdiff_t n = displays.size(); // convert to signed
    verify(n > 0);

    overlaps.resize(displays.size());
    std::transform(std::begin(displays), std::end(displays), 
        std::begin(overlaps), 
        [&window](const Bounds &display) { 
            return display.overlap_area(window);
        });
    
    // always valid if n > 0
    const auto it = std::max_element(std::begin(overlaps),
                                     std::end(overlaps));
    
    // if overlap is all 0, idx = 0 (intended)
    auto idx = std::distance(std::begin(overlaps), it);

    if (idx_offset < 0)
        idx_offset += n * (-idx_offset / n + 1); // force positive
    idx = (idx + idx_offset) % n; 

    Bounds bounds(displays[idx]);
    bounds.x += std::get<0>(margins); // l
    bounds.y += std::get<2>(margins); // t
    bounds.w -= std::get<0>(margins) + std::get<1>(margins); // l + r
    bounds.h -= std::get<2>(margins) + std::get<3>(margins); // t + b

    // r & b may not be respected if too large
    bounds.w = std::max(bounds.w, 1);
    bounds.h = std::max(bounds.h, 1);

    return bounds;
}

const std::vector<Bounds>& DisplayList::get_vec()
{
    return displays;
}

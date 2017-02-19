#ifndef CONFIG_H_
#define CONFIG_H_

#include "common.h"
#include "Bounds.h"

#include <fstream>
#include <vector>
#include <sstream>

class Config
{
public:
    bool set_file(CSR filename); // false if not writable to filename
                                 // should not use any of below if failed
    std::pair<STR, bool> get_value(CSR key); // false if failed

    bool set_value(CSR key, CSR value); // false if failed

protected:
    STR filename;
};

class TileConfig : public Config
{
public:
    std::pair<int, bool> get_unit_width();
    //                    l    r    t    b
    std::pair<std::tuple<int, int, int, int>, bool> get_margins();

    // only using win.bounds, win.owner, win.index
    bool set_last_bounds(const std::vector<Window> &windows);
    std::pair<std::vector<Window>, bool> get_last_bounds();
};

#endif /* CONFIG_H_ */

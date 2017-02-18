#ifndef CONFIG_H_
#define CONFIG_H_

#include "common.h"

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
    //                  name  idx   x    y    w    h
    std::pair<std::tuple<STR, int, int, int, int, int>, bool> 
        get_last_bounds();
    //                             name  idx   x    y    w    h
    bool set_last_bounds(std::tuple<STR, int, int, int, int, int> b);

};

#endif /* CONFIG_H_ */

#ifndef WINDOWLIST_H_
#define WINDOWLIST_H_

#include "Bounds.h"

#include <vector>

class WindowList
{
public:
    void add_window(CSR owner, Window&& window);
    
    // nullptr = failed to find or multiple app_name match
    const std::pair<STR, std::vector<Window>>* find(CSR substr);
   
    bool print(CSR substr); // false if failed

    const std::vector<std::pair<STR, std::vector<Window>>>& get_vec();

private:
    // (owner, vec(window))
    std::vector<std::pair<STR, std::vector<Window>>> windows;
};

#endif /* WINDOWLIST_H_ */

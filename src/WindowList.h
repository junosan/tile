#ifndef WINDOWLIST_H_
#define WINDOWLIST_H_

#include "Bounds.h"

#include <vector>

class WindowList
{
public:
    void add_window(Bounds bounds, CSR owner, CSR title, int pid);
    
    // store pointer to focused app's pair(owner, vec)
    // if any app precedes focused in windows_ordered, erase
    bool set_focused_app(int pid); // false if invalid pid

    // nullptr = failed to find or multiple app_name match
    const std::pair<STR, std::vector<Window>>* find(CSR substr) const;
   
    bool print(CSR substr) const; // false if failed

    const std::vector<Window>& get_vec() const;

private:
    // map (owner, vec(window))
    std::vector<std::pair<STR, std::vector<Window>>> windows;
    std::vector<Window> windows_ordered;

    const std::pair<STR, std::vector<Window>>* focused_pair_ptr = nullptr;
};

#endif /* WINDOWLIST_H_ */

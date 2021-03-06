#include "WindowList.h"

#include "format.h"

void WindowList::add_window
    (Bounds bounds, CSR owner, CSR title, int pid)
{
    Window window{bounds, owner, title, 0, pid}; // index filled below

    auto it  = std::begin(windows);
    auto end = std::end(windows);
    for (; it != end; ++it)
    {
        if (owner == it->first)
        {
            window.index = it->second.size();
            it->second.emplace_back(window);
            break;
        }
    }
    if (it == end)
    {
        std::vector<Window> vec;
        window.index = 0;
        vec.emplace_back(window);
        windows.emplace_back(owner, std::move(vec));
    }

    windows_ordered.emplace_back(std::move(window));
}

bool WindowList::set_focused_app(int pid)
{
    auto begin = std::begin(windows_ordered);
    auto end = std::end(windows_ordered);
    auto it_focused = begin;
    for (; it_focused != end; ++it_focused)
    {
        if (it_focused->pid == pid)
            break;
    }

    if (it_focused == end)
        return false; // invalid call
    
    for (const auto &pair : windows)
    {
        if (pair.second[0].pid == pid)
        {
            focused_pair_ptr = &pair;
            break;
        }
    }

    windows_ordered.erase(begin, it_focused);

    return true;
}

const std::pair<STR, std::vector<Window>>* WindowList::find(CSR substr) const
{
    if (substr.empty() == true && focused_pair_ptr != nullptr)
        return focused_pair_ptr;

    const std::pair<STR, std::vector<Window>>* pair_ptr = nullptr;

    STR substr_l(substr);
    std::transform(substr_l.begin(), substr_l.end(),
                    substr_l.begin(), ::tolower);
    auto len = substr_l.size();

    bool found(false);
    for (auto it = std::begin(windows), end = std::end(windows);
            it != end; ++it)
    {
        STR owner_substr_l(it->first.substr(0, len));
        std::transform(owner_substr_l.begin(), owner_substr_l.end(),
                        owner_substr_l.begin(), ::tolower);
        
        if (substr_l == owner_substr_l)
        {
            if (found == true)
            {
                pair_ptr = nullptr;
                break;
            }
            pair_ptr = &*it;
            found = true;
        }
    }

    return pair_ptr;
}

bool WindowList::print(CSR substr) const
{
    if (substr.empty() == true) // print all, ordered
    {
        for (const auto &window : windows_ordered)
            std::cout << fmt_window(window);
    }
    else // print specified app
    {
        auto pair_ptr = find(substr);
        if (pair_ptr != nullptr)
        {
            for (const auto &window : pair_ptr->second)
                std::cout << fmt_window(window);
        }
        else
            return false;
    }
    return true;
}

const std::vector<Window>& WindowList::get_vec() const
{
    return windows_ordered;
}

#include "WindowList.h"

#include "format.h"

void WindowList::add_window(CSR owner, Window&& window)
{
    auto it  = std::begin(windows);
    auto end = std::end(windows);
    for (; it != end; ++it)
    {
        if (owner == it->first)
        {
            it->second.emplace_back(std::move(window));
            break;
        }
    }
    if (it == end)
    {
        std::vector<Window> vec;
        vec.emplace_back(std::move(window));
        windows.emplace_back(owner, std::move(vec));
    }
}

const std::pair<STR, std::vector<Window>>* WindowList::find(CSR substr)
{
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

bool WindowList::print(CSR substr)
{
    if (substr.empty() == true) // print all
    {
        for (const auto &owner_window : windows)
        {
            auto i = 0u;
            for (const auto &window : owner_window.second)
                std::cout << fmt_window(owner_window.first, i++, window);
        }
    }
    else // print specified app
    {
        auto pair_ptr = find(substr);
        if (pair_ptr != nullptr)
        {
            auto i = 0u;
            for (const auto &window : pair_ptr->second)
                std::cout << fmt_window(pair_ptr->first, i++, window);
        }
        else
            return false;
    }
    return true;
}

const std::vector<std::pair<STR, std::vector<Window>>>& WindowList::get_vec()
{
    return windows;
}

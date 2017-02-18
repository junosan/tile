#ifndef TILE_H_
#define TILE_H_

#ifdef NDEBUG
    #define verify(expression) ((void)(expression))
#else
    #include <cassert>
    #define verify(expression) assert(expression)
#endif /* NDEBUG */

#include <string>
using STR  = std::string;
using CSTR = const STR;

#include <tuple>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

class ArgParser {
public:
    enum class action { invalid, help, list, undo, move, tile };
    enum class v_cmd  { none, f, t, b, y, h, n, j, k };
    enum class m_dir  { none, prev, next };

    struct Args
    {
        action action;

        // $l and $r in documentation
        //   l  > r >= 0    invalid
        //   l == r >= 0    silent no-op
        //   l  < 0         keep current pos_x
        //   r  < 0         extend to effective screen width
        struct {
            float l, r;
        } h_cmd;
        v_cmd v_cmd;
        m_dir m_dir;

        STR substr;
        std::size_t index;

        Args() : action{action::invalid},
                h_cmd{0.f, 0.f}, v_cmd{v_cmd::none}, m_dir{m_dir::none},
                index{0} {}
    };

    Args parse(int argc, const char *argv[])
    {
        Args args;

        if (argc > 4)
            return args;

        // store given or default values for arguments
        STR arg1   = (argc > 1 ? STR(argv[1]) : STR(""));
        STR substr = (argc > 2 ? STR(argv[2]) : STR(""));

        std::size_t index(0);
        if (argc > 3)
        {
            int i(-1);
            try {
                i = std::stoi(argv[3]);
            } catch(...) {
                return args;
            }
            if (i >= 0)
                index = (std::size_t)i;
            else
                return args;
        }
        
        // parse
        if (arg1 == "help" || arg1 == "--help")
        {
            args.action = action::help;
        }

        else if (argc <= 1 || (arg1 == "list" && argc < 4))
        {
            args.action = action::list;
            args.substr = std::move(substr);
        }

        else if (argc == 2 && arg1 == "undo")
        {
            args.action = action::undo;
        }

        else if (arg1 == "[" || arg1 == "]")
        {
            args.action = action::move;
            args.m_dir  = (arg1 == "[" ? m_dir::prev : m_dir::next);
            args.substr = std::move(substr);
            args.index  = index;
        }

        else
        {
            verify(arg1.size() > 0);

            auto v_pos = arg1.find_first_of("ftbyhnjk");
            
            if (v_pos == arg1.size() - 1) // assumes size > 0
            {
                switch (arg1[v_pos])
                {
                    case 'f': args.v_cmd = v_cmd::f; break;
                    case 't': args.v_cmd = v_cmd::t; break;
                    case 'b': args.v_cmd = v_cmd::b; break;
                    case 'y': args.v_cmd = v_cmd::y; break;
                    case 'h': args.v_cmd = v_cmd::h; break;
                    case 'n': args.v_cmd = v_cmd::n; break;
                    case 'j': args.v_cmd = v_cmd::j; break;
                    case 'k': args.v_cmd = v_cmd::k; break;
                    default : verify(false);
                }
            }
            // v_cmd must be 0 or 1 character
            else if (v_pos < arg1.size() - 1) // assumes size > 0
                return args;
            
            STR h = arg1.substr(0, v_pos);

            if (h.empty() == false)
            {
                if (std::string::npos != h.find_first_not_of("0123456789.-"))
                    return args;
                
                // may not have negative numbers
                if (std::count(std::begin(h), std::end(h), '-') > 1)
                    return args;
                
                auto dash_pos = h.find_first_of('-');

                if (std::string::npos == dash_pos) // unit width
                {
                    try {
                        args.h_cmd.l = std::stof(h);
                    } catch(...) {
                        return args;
                    }
                    args.h_cmd.r = args.h_cmd.l + 1;
                }
                else // general width
                {
                    STR l = h.substr(0, dash_pos);
                    STR r = h.substr(dash_pos + 1); // pos + 1 == size is ok
                    
                    if (l.empty() == true)
                        args.h_cmd.l = -1.f;
                    else
                    {
                        try {
                            args.h_cmd.l = std::stof(l);
                        } catch(...) {
                            return args;
                        }
                    }

                    if (r.empty() == true)
                        args.h_cmd.r = -1.f;
                    else
                    {
                        try {
                            args.h_cmd.r = std::stof(r);
                        } catch(...) {
                            return args;
                        }
                    }

                    if (args.h_cmd.l > args.h_cmd.r && args.h_cmd.r >= 0.f)
                        return args;
                }
            }

            args.action = action::tile;
            args.substr = std::move(substr);
            args.index  = index;
        }

        return args;
    }

    void print_usage()
    {
        std::cout << "USAGE: tile [command] [app_name] [window_index]\n";
        // add full later
    }
};

struct Bounds
{
    int x, y, w, h; // use aggregate initialization

    int overlap_area(Bounds other) const
    {
        auto overlap_len = [](int o1, int s1, int o2, int s2) {
            return (o1 < o2 ? std::min(std::max(o1 + s1 - o2, 0), s2) :
                              std::min(std::max(o2 + s2 - o1, 0), s1));
        };
        return overlap_len(x, w, other.x, other.w) *
               overlap_len(y, h, other.y, other.h);
    }

    static std::pair<int, int> // Bounds::sub(x, w, ...) or (y, h, ...)
        sub(int origin, int size, float unit, float begin, float end)
    {
        int rel_orig = std::min(static_cast<int>(std::round(unit * begin)),
                                size);
        int rel_term = std::min(static_cast<int>(std::round(unit * end)),
                                size);
        rel_orig = std::min(rel_orig, rel_term - 1);

        return std::make_pair(origin + rel_orig, rel_term - rel_orig);
    }
};

inline std::ostream& operator<<(std::ostream &os, const Bounds &b) {
    return os << "origin(" << std::setw(5) << b.x << ","
                           << std::setw(5) << b.y << ")"
                 " size("  << std::setw(5) << b.w << ","
                           << std::setw(5) << b.h << ")";
}


// formatting classes
#define FMT_DEFINE(name_, w_, vtype_, itype_, isuffix_) \
    struct name_ { \
        constexpr static int w = w_; \
        vtype_ val; \
        name_ (itype_ in) : val(in isuffix_) {} \
    };

FMT_DEFINE(fmt_owner, 24, STR, CSTR&, );
FMT_DEFINE(fmt_title, 50, STR, CSTR&, );

#undef FMT_DEFINE

// 24 + 5 + 50 = 79
inline std::ostream& operator<<(std::ostream &os, const fmt_owner &in) {
    return os << std::setw(fmt_owner::w)
              << in.val.substr(0, fmt_owner::w);
}

inline std::ostream& operator<<(std::ostream &os, const fmt_title &in) {
    return os << std::left << std::setw(fmt_title::w)
              << in.val.substr(0, fmt_title::w) << std::right;
}


struct Window
{
    Bounds bounds;
    STR title;
    unsigned int pid;

    void print(CSTR & owner, std::size_t index) const
    {
        std::cout << fmt_owner(owner) << " " << index << " : "
                << fmt_title(title) << '\n'
                << std::setw(fmt_owner::w) << ""
                << "     " << bounds << '\n' ;
    }
};

class WindowList
{
public:
    void add_window(CSTR &owner, Window&& window)
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
    
    // nullptr = failed to find or multiple app_name match
    const std::pair<STR, std::vector<Window>>* find(CSTR &substr)
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
    
    bool print(CSTR &substr) // false if failed
    {
        if (substr.empty() == true) // print all
        {
            for (const auto &owner_window : vec())
            {
                auto i = 0u;
                for (const auto &window : owner_window.second)
                    window.print(owner_window.first, i++);
            }
        }
        else // print specified app
        {
            auto pair_ptr = find(substr);
            if (pair_ptr != nullptr)
            {
                auto i = 0u;
                for (const auto &window : pair_ptr->second)
                    window.print(pair_ptr->first, i++);
            }
            else
                return false;
        }
        return true;
    }

    const std::vector<std::pair<STR, std::vector<Window>>>& vec()
    {
        return windows;
    }    

private:
    // (owner, vec(window))
    std::vector<std::pair<STR, std::vector<Window>>> windows;
};

class DisplayList
{
public:
    DisplayList() : margins{0, 0, 0, 0} {}

    void set_margins(std::tuple<int, int, int, int> margins)
    {
        this->margins = margins;
    }

    void add_display(Bounds display)
    {
        displays.emplace_back(display);
    }

    // find display based on most overlap
    // return display (+ idx_offset) after margins applied
    Bounds target_display(Bounds window, std::ptrdiff_t idx_offset = 0)
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
        bounds.x += std::get<0>(margins);
        bounds.y += std::get<2>(margins);
        bounds.w -= std::get<0>(margins) + std::get<1>(margins);
        bounds.h -= std::get<2>(margins) + std::get<3>(margins);

        // r & b may not be respected if too large
        bounds.w = std::max(bounds.w, 1);
        bounds.h = std::max(bounds.h, 1);

        return bounds;
    }

    const std::vector<Bounds>& vec()
    {
        return displays;
    }

private:
    //          l    r    t    b
    std::tuple<int, int, int, int> margins;
    std::vector<Bounds> displays; // before margins
    std::vector<int>    overlaps;
};


#include <fstream>
#include <sstream>

class Config
{
public:
    bool set_file(CSTR &filename) // false if not writable to filename
    {                             // should not use any of below if failed
        std::ofstream ofs(filename, std::ios::app);
        bool valid = ofs.is_open();
        if (valid == true)
            this->filename = filename;
        return valid;
    }

    std::pair<STR, bool> get_value(CSTR &key) // false if failed
    {
        std::ifstream ifs(filename);
        if (ifs.is_open() == true)
        {
            for (STR line; std::getline(ifs, line);)
            {
                auto space = line.find_first_of(" \t");
                if (space != std::string::npos &&
                    line.substr(0, space) == key)
                {
                    auto val = line.find_first_not_of(" \t", space);
                    if (val != std::string::npos)
                        return std::make_pair(line.substr(val), true);
                }
            }
        }
        return std::make_pair(STR(""), false);
    }

    bool set_value(CSTR &key, CSTR &value) // false if failed
    {
        std::vector<std::pair<STR, STR>> map;

        // read all
        {
            std::ifstream ifs(filename);
            if (ifs.is_open() == false)
                return false;
            for (STR line; std::getline(ifs, line);)
            {
                auto space = line.find_first_of(" \t");
                if (space == std::string::npos)
                    continue;
                auto val = line.find_first_not_of(" \t", space);
                if (val != std::string::npos)
                    map.emplace_back(line.substr(0, space),
                                     line.substr(val));
            }
        } // file closed

        // replace or add
        {
            auto it  = std::begin(map);
            auto end = std::end(map);
            for (; it != end; ++it)
            {
                if (it->first == key)
                {
                    it->second = value;
                    break;
                }
            }
            if (it == end)
                map.emplace_back(key, value);
        }

        // rewrite
        {
            std::ofstream ofs(filename, std::ios::trunc);
            if (ofs.is_open() == false)
                return false;
            for (const auto &pair : map)
                ofs << pair.first << ' ' << pair.second << '\n';
        } // file closed

        return true;
    }

protected:
    STR filename;
};

class TileConfig : public Config
{
public:
    std::pair<int, bool> get_unit_width()
    {
        verify(filename.empty() == false);

        STR val;
        bool valid;
        std::tie(val, valid) = get_value("unit_width");
        if (valid == false)
        {
            val = STR("585");
            set_value("unit_width", val);
            valid = true;
        }

        int unit_width(0);
        try {
            unit_width = std::stoi(val);
        } catch(...) {
            valid = false;
        }
        if (unit_width <= 0)
            valid = false;

        return std::make_pair(unit_width, valid);
    }

    std::pair<std::tuple<int, int, int, int>, bool> get_margins()
    {
        verify(filename.empty() == false);

        //          l    r    t    b
        std::tuple<int, int, int, int> m{0, 0, 0, 0};

        STR val;
        bool valid;
        std::tie(val, valid) = get_value("margin");
        if (valid == false)
        {
            val = STR("0 0 0 0");
            set_value("margin", val);
            valid = true;
        }

        std::stringstream ss(val);
        ss >> std::get<0>(m) >> std::get<1>(m)
           >> std::get<2>(m) >> std::get<3>(m);
        if (ss.fail() == true)
            valid = false;

        return std::make_pair(m, valid);
    }

    std::pair<std::tuple<STR, int, int, int, int, int>, bool> get_last_bounds()
    {
        verify(filename.empty() == false);

        //        name  idx   x    y    w    h
        std::tuple<STR, int, int, int, int, int> b{"", 0, 0, 0, 0, 0};

        STR val;
        bool valid;
        std::tie(val, valid) = get_value("last_bounds");
        if (valid == true)
        {
            if (std::count(std::begin(val), std::end(val), '\037') == 5)
            {
                auto first = val.find_first_of('\037');
                std::get<0>(b) = val.substr(0, first);

                std::transform(std::begin(val) + first, std::end(val),
                    std::begin(val) + first,
                    [](char c){ return (c != '\037' ? c : ' '); });
                
                std::stringstream ss(val.substr(first));
                ss >> std::get<1>(b) >> std::get<2>(b) >> std::get<3>(b)
                   >> std::get<4>(b) >> std::get<5>(b);
                if (ss.fail() == true)
                    valid = false;
            }
            else
                valid = false;
        }

        // window_index, w, h
        if (std::get<1>(b) < 0 || std::get<4>(b) <= 0 || std::get<5>(b) <= 0)
            valid = false;

        return std::make_pair(b, valid);
    }

    bool set_last_bounds(std::tuple<STR, int, int, int, int, int> b)
    {
        verify(filename.empty() == false);

        // window_index, w, h
        bool success = (std::get<1>(b) >= 0 &&
                        std::get<4>(b) > 0 && std::get<5>(b) > 0);
        if (success == true)
        {
            success = set_value("last_bounds",
                        std::get<0>(b) + '\037'
                      + std::to_string(std::get<1>(b)) + '\037'
                      + std::to_string(std::get<2>(b)) + '\037'
                      + std::to_string(std::get<3>(b)) + '\037'
                      + std::to_string(std::get<4>(b)) + '\037'
                      + std::to_string(std::get<5>(b)) );
        }
        return success;
    }
};

#endif /* TILE_H_ */

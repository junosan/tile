#include "Config.h"

bool Config::set_file(CSR filename)
{
    std::ofstream ofs(filename, std::ios::app);
    bool valid = ofs.is_open();
    if (valid == true)
        this->filename = filename;
    return valid;
}

std::pair<STR, bool> Config::get_value(CSR key)
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

bool Config::set_value(CSR key, CSR value)
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

std::pair<int, bool> TileConfig::get_unit_width()
{
    verify(filename.empty() == false);

    STR val;
    bool valid;
    std::tie(val, valid) = get_value("unit_width");
    if (valid == false)
    {
        val = STR("570"); // default value
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

std::pair<std::tuple<int, int, int, int>, bool> TileConfig::get_margins()
{
    verify(filename.empty() == false);

    //          l    r    t    b
    std::tuple<int, int, int, int> m{0, 0, 0, 0};

    STR val;
    bool valid;
    std::tie(val, valid) = get_value("margin_lrtb");
    if (valid == false)
    {
        val = STR("0 0 0 0"); // default value
        set_value("margin_lrtb", val);
        valid = true;
    }

    std::stringstream ss(val);
    ss >> std::get<0>(m) >> std::get<1>(m)
        >> std::get<2>(m) >> std::get<3>(m);
    if (ss.fail() == true)
        valid = false;

    return std::make_pair(m, valid);
}

bool TileConfig::set_last_bounds(const std::vector<Window> &windows)
{
    verify(filename.empty() == false);

    STR val;
    bool valid(true);

    for (const auto &win : windows)
    {
        if (win.bounds.w <= 0 || win.bounds.h <= 0)
        {
            valid = false;
            break;
        }

        val += win.owner + '\037' + std::to_string(win.index) + '\037'
                + std::to_string(win.bounds.x) + '\037' 
                + std::to_string(win.bounds.y) + '\037'
                + std::to_string(win.bounds.w) + '\037'
                + std::to_string(win.bounds.h) + '\036';
    }

    if (val.empty() == true)
        valid = false;
    else
        val.pop_back();

    if (valid == true)
        valid = set_value("last_bounds", val);

    return valid;
}

std::pair<std::vector<Window>, bool> TileConfig::get_last_bounds()
{
    verify(filename.empty() == false);

    std::vector<Window> windows;
    
    STR val;
    bool valid;
    std::tie(val, valid) = get_value("last_bounds");
    if (valid == true)
    {
        std::stringstream ss(val);
        for (STR rec; std::getline(ss, rec, '\036');)
        {
            if (std::count(std::begin(rec), std::end(rec), '\037') == 5)
            {
                Window win;

                auto first = rec.find_first_of('\037');
                win.owner = rec.substr(0, first);

                std::transform(std::begin(rec) + first, std::end(rec),
                    std::begin(rec) + first,
                    [](char c){ return (c != '\037' ? c : ' '); });
                
                std::stringstream ss(rec.substr(first));
                int index(-1);

                ss >> index >> win.bounds.x >> win.bounds.y
                            >> win.bounds.w >> win.bounds.h;
                if (ss.fail() == true || index < 0 ||
                    win.bounds.w <= 0 || win.bounds.h <= 0)
                {
                    valid = false;
                    break;
                }

                win.index = (std::size_t)index;
                windows.emplace_back(std::move(win));
            }
            else
            {
                valid = false;
                break;
            }
        }
    }

    return std::make_pair(windows, valid); // RVO
}

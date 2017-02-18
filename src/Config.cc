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

std::pair<std::tuple<STR, int, int, int, int, int>, bool> 
    TileConfig::get_last_bounds()
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

bool TileConfig::set_last_bounds(std::tuple<STR, int, int, int, int, int> b)
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

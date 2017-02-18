
#include "apple_api.h"
#include "ArgParser.h"
#include "Config.h"

#import <Foundation/Foundation.h>

#include <iostream>
#include <cstdlib>

int main(int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    auto clean_exit = [&pool](int exit_code) {
        [pool drain];
        return exit_code;
    };

    if (false == apple_api::enable_accessibility_api())
    {
        std::cerr << "Accessibility API unavailable\n";
        return clean_exit(1);
    }
    

    ArgParser arg_parser;
    auto args = arg_parser.parse(argc, argv);

    if (args.action == ArgParser::action::invalid)
    {
        std::cerr << "Invalid arguments\n";
        return clean_exit(1);
    }

    if (args.action == ArgParser::action::help)
    {
        arg_parser.print_usage();
        return clean_exit(0);
    }

    if (args.action == ArgParser::action::list)
    {
        if (false == apple_api::build_lists().first.print(args.substr))
        {
            std::cerr << "Cannot find unique app name starting with '"
                      << args.substr << "'\n";
            return clean_exit(1);
        }
        return clean_exit(0);
    }


    TileConfig config;
    int unit_width;
    std::tuple<int, int, int, int> margins;
    {
        STR tilerc(STR(std::getenv("HOME")) + "/.tilerc");

        if (false == config.set_file(tilerc))
        {
            std::cerr << "Cannot obtain write access to " << tilerc << '\n';
            return clean_exit(1);
        }

        bool valid_u, valid_m;
        std::tie(unit_width, valid_u) = config.get_unit_width();
        std::tie(margins   , valid_m) = config.get_margins();
        
        if (valid_u == false || valid_m == false)
        {
            std::cerr << "Invalid settings in " << tilerc << '\n';
            return clean_exit(1);
        }
    }
    

    if (args.action == ArgParser::action::undo)
    {
        //        name  idx   x    y    w    h
        std::tuple<STR, int, int, int, int, int> b;
        bool valid;
        std::tie(b, valid) = config.get_last_bounds();

        if (valid == true)
        {
            valid = false;
            auto win_list = apple_api::build_lists().first;
            auto pair_ptr = win_list.find(std::get<0>(b)); // life ~ win_list

            if (pair_ptr != nullptr &&
                std::get<1>(b) < pair_ptr->second.size())
            {
                const auto &win = pair_ptr->second[std::get<1>(b)];
                valid = apple_api::apply_bounds(win, 
                    Bounds{std::get<2>(b), std::get<3>(b),
                           std::get<4>(b), std::get<5>(b)});
                if (valid == true)
                    config.set_last_bounds({pair_ptr->first, std::get<1>(b),
                                            win.bounds.x, win.bounds.y,
                                            win.bounds.w, win.bounds.h});
            }
        }

        if (valid == false)
        {
            std::cerr << "Undo failed\n";
            return clean_exit(1);
        }

        return clean_exit(0);
    }


    auto win_disp = (args.substr.empty() == true 
                        ? apple_api::build_lists(true)
                        : apple_api::build_lists());
    win_disp.second.set_margins(margins);
    
    auto pair_ptr = win_disp.first.find(args.substr); // life ~ win_disp.first

    if (pair_ptr == nullptr)
    {
        std::cerr << "Cannot find unique app name starting with '"
                    << args.substr << "'\n";
        return clean_exit(1);
    }
    
    if (args.index >= pair_ptr->second.size())
    {
        std::cerr << "Invalid window index " << args.index << '\n';
        return clean_exit(1);
    }

    const auto &win = pair_ptr->second[args.index];
    Bounds bounds(win.bounds); // win.bounds (old) -> bounds (new)

    if (args.action == ArgParser::action::move)
    {
        Bounds cur_disp = win_disp.second.target_display(win.bounds);

        std::ptrdiff_t offset(args.m_dir == ArgParser::m_dir::prev ? -1 : 1);
        Bounds new_disp = win_disp.second.target_display(win.bounds, offset);
        
        // attempt to preserve relative origin
        bounds.x += new_disp.x - cur_disp.x;
        bounds.y += new_disp.y - cur_disp.y;

        bounds = new_disp.fit(bounds);
    }

    if (args.action == ArgParser::action::tile)
    {
        Bounds disp = win_disp.second.target_display(win.bounds);
        
        bounds = disp.h_sub(bounds, unit_width, args.h_cmd.l, args.h_cmd.r);
        bounds = disp.v_sub(bounds, args.v_cmd);
    }

    if (true == apple_api::apply_bounds(win, bounds))
    {
        config.set_last_bounds({pair_ptr->first, args.index,
                                win.bounds.x, win.bounds.y,
                                win.bounds.w, win.bounds.h});
    }
    else
    {
        std::cerr << "Window moved or disappeared while trying to tile\n";
        return clean_exit(1);
    }

    return clean_exit(0);
}

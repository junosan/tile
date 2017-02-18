
#include "apple_api.h"
#include "ArgParser.h"
#include "Config.h"

#import <Foundation/Foundation.h>

#include <iostream>
#include <cstdlib>

// take this out when stuff moved
#include <cmath>

int main(int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    auto clean_exit = [&pool](int exit_code) {
        [pool drain];
        return exit_code;
    };

    if (false == apple_api::enable_accessibility_api())
        return clean_exit(1);
    
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

            // note: win_list must outlive pair_ptr
            auto pair_ptr = win_list.find(std::get<0>(b));

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

    if (args.action == ArgParser::action::move)
    {
        // to be implemented
        return clean_exit(0);
    }

    if (args.action == ArgParser::action::tile)
    {
        auto win_disp = (args.substr.empty() == true
                            ? apple_api::build_lists(true)
                            : apple_api::build_lists());
        
        // note: win_disp must outlive pair_ptr
        auto pair_ptr = win_disp.first.find(args.substr);
        
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

        win_disp.second.set_margins(margins);
        Bounds disp = win_disp.second.target_display(win.bounds);
        
        Bounds bounds(win.bounds); // win.bounds (old), bounds (new)

        // see comments in ArgParser::Args
        if (args.h_cmd.l != args.h_cmd.r || args.h_cmd.r < 0.f)
        {
            int rel_orig = std::min(disp.w,
                (args.h_cmd.l >= 0.f
                    ? static_cast<int>(std::round(args.h_cmd.l * unit_width))
                    : bounds.x - disp.x));
            int rel_term = std::min(disp.w,
                (args.h_cmd.r >= 0.f
                    ? static_cast<int>(std::round(args.h_cmd.r * unit_width))
                    : disp.w));
            rel_orig = std::min(rel_orig, rel_term - 1);

            bounds.x = disp.x + rel_orig;
            bounds.w = rel_term - rel_orig;
        }

        if (args.v_cmd != ArgParser::v_cmd::none)
        {
            float unit_height;
            switch (args.v_cmd)
            {
                case ArgParser::v_cmd::f: unit_height = disp.h; break;
                case ArgParser::v_cmd::t:
                case ArgParser::v_cmd::b: unit_height = disp.h / 2.f; break;
                case ArgParser::v_cmd::y:
                case ArgParser::v_cmd::h:
                case ArgParser::v_cmd::n:
                case ArgParser::v_cmd::j:
                case ArgParser::v_cmd::k: unit_height = disp.h / 3.f; break;
                default: verify(false);
            }

            float begin;
            switch (args.v_cmd)
            {
                case ArgParser::v_cmd::f:
                case ArgParser::v_cmd::t:
                case ArgParser::v_cmd::y:
                case ArgParser::v_cmd::k: begin = 0.f; break;
                case ArgParser::v_cmd::b:
                case ArgParser::v_cmd::h:
                case ArgParser::v_cmd::j: begin = 1.f; break;
                case ArgParser::v_cmd::n: begin = 2.f; break;
                default: verify(false);
            }

            float end;
            switch (args.v_cmd)
            {
                case ArgParser::v_cmd::f:
                case ArgParser::v_cmd::t:
                case ArgParser::v_cmd::y: end = 1.f; break;
                case ArgParser::v_cmd::b:
                case ArgParser::v_cmd::h:
                case ArgParser::v_cmd::k: end = 2.f; break;
                case ArgParser::v_cmd::n:
                case ArgParser::v_cmd::j: end = 3.f; break;
                default: verify(false);
            }

            std::tie(bounds.y, bounds.h) = 
                Bounds::sub(disp.y, disp.h, unit_height, begin, end);
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

    return clean_exit(0);
}


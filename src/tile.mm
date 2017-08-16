
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
        auto wins_valid = config.get_last_bounds();
        bool success(wins_valid.second);

        if (success == true)
        {
            auto win_list = apple_api::build_lists().first;
            std::vector<Window> old_wins;

            for (const auto &new_win : wins_valid.first)
            {
                auto pair_ptr = win_list.find(new_win.owner);

                if (pair_ptr != nullptr &&
                    new_win.index < pair_ptr->second.size())
                {
                    const auto &win = pair_ptr->second[new_win.index];

                    if (true == apple_api::apply_bounds(win, new_win.bounds))
                        old_wins.emplace_back(win);
                    else
                        success = false;
                }
                else
                    success = false;
            }

            config.set_last_bounds(old_wins);
        }

        if (success == false)
        {
            std::cerr << "Undo failed or finished incompletely\n";
            return clean_exit(1);
        }

        return clean_exit(0);
    }

    // snap, move, tile
    auto win_disp = apple_api::build_lists();
    win_disp.first.set_focused_app(apple_api::get_menu_bar_owner_pid());
    win_disp.second.set_margins(margins);

    if (args.action == ArgParser::action::snap)
    {
        const auto &windows = win_disp.first.get_vec(); 
        if (windows.empty() == true)
        {
            std::cerr << "No open windows to snap\n";
            return clean_exit(1);
        }

        auto n_win = std::min(args.index, windows.size());

        Bounds blob(windows[0].bounds); // grows as more windows are attached
        Bounds disp = win_disp.second.target_display(blob);

        auto mid_x = [](Bounds b) { return (float)b.x + (float)b.w / 2.f; };

        if (n_win == 0) // shift to left or right side of to-be-centered window
            blob.x = mid_x(disp) +
                     (mid_x(blob) > mid_x(disp) ? -1.5f : 0.5f) * blob.w;

        if (n_win == 1) // shift to just outside left or right edge
            blob.x = disp.x + (mid_x(blob) > mid_x(disp) ? disp.w : -blob.w);

        float pivot = mid_x(blob);
        
        std::vector<Window> snapped_wins;
        bool success(true);

        auto lb = (n_win > 1 ? 1u        : 0u);
        auto ub = (n_win > 1 ? n_win - 1 : 0u);
        for (auto i = lb; i <= ub; ++i)
        {
            Bounds bounds = windows[i].bounds;
            bounds = blob.snap(bounds, mid_x(bounds) > pivot 
                                ? Bounds::snap_dir::r : Bounds::snap_dir::l);
            
            if (disp.overlap_area(bounds) == 0)
                continue;
            
            if (true == apple_api::apply_bounds(windows[i], bounds))
            {
                blob.attach(bounds);
                snapped_wins.emplace_back(windows[i]);
            }
            else
                success = false;
        }

        config.set_last_bounds(snapped_wins);

        if (success == false)
        {
            std::cerr << "Snap failed or finished incompletely\n";
            return clean_exit(1);
        }

        return clean_exit(0);
    }


    // move, tile
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
        config.set_last_bounds({1, win});
    }
    else
    {
        std::cerr << "Window moved or disappeared while trying to tile\n";
        return clean_exit(1);
    }

    return clean_exit(0);
}

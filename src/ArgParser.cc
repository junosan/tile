#include "ArgParser.h"

#include <iostream>

ArgParser::Args ArgParser::parse(int argc, const char *argv[])
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

void ArgParser::print_usage()
{
    std::cout << R"USAGE_TEXT(
tile
    equivalent to 'tile list'

tile help
    display this page

tile undo
    swap current_bounds & last_bounds

tile list [app_name] ([ ]: optional)
    [app_name]  - case insensitive substring (at index 0) of an app name
                - if there are whitespaces, enclose app_name with '' or ""
                - no-op if number of matches is not 1
        (none)  list currently opened app names and their windows
         name   list the named app's windows

tile [command] [app_name] [window_index] (all positional arguments)
    [command] = [h_cmd][v_cmd]
                - dimensions specified are with respect to the display
                  with the most overlap area with the chosen window
        [h_cmd]     - nonnegative numbers, possibly decimal
                    - no-op if $l >= $r
            (none)          keep current horizontal position & width
            $l              pos_x = $l unit, width = 1 unit
            [$l]-[$r]
                $l  (none)  pos_x = keep current 
                    number  pos_x = number unit
                $r  (none)  width extends to effective screen width
                    number  width = (number unit) - pos_x
        [v_cmd]       
            (none)  keep current vertical position & height
            f       full height
            t/b     top half, bottom half
            y/h/n   top third, middle third, bottom third
            j/k     (bottom + middle)/(top + middle) thirds
    [command] = [/] move to prev/next display
    [app_name]
        (none)      use the window with current focus
         name       use the named app (same rules as app_name above)
    [window_index]
        (none)      equivalent to 0
        number      enumerated index in 'tile list' or 'tile list app_name'

$HOME/.tilerc
    unit_width      nonnegative integer (default: 570)
    margin_lrtb     integers for left, right, top, bottom (default: 0 0 0 0)
    last_bounds     not intended for human editing
)USAGE_TEXT" << '\n';

}

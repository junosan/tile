
// clang++ -framework Carbon -framework Cocoa -std=c++14 -Wall -O2 -o tile tile.mm

#include "tile.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

std::pair<WindowList, DisplayList> build_lists(bool front_only = false)
{
    WindowList  window_list;
    DisplayList display_list;

    NSArray *window_arr = (NSArray *)CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    
    bool got_first(false);

    for (NSDictionary *window in window_arr)
    {
        const char *str = [window[(id)kCGWindowOwnerName] UTF8String];
        STR owner(str != nullptr ? str : "");
        
        str = [window[(id)kCGWindowName] UTF8String];
        STR title(str != nullptr ? str : "");

        if (owner == "Dock") continue;
        if (owner == "Finder" && title.empty() == true) continue;
        
        bool is_display = (owner == "Window Server" && title == "Desktop");
        unsigned int pid = [window[(id)kCGWindowOwnerPID] unsignedIntValue];
        
        if (is_display == false)
        {
            if (front_only == true && got_first == true)
                continue;
            
            // skip Menu Bar and icons on it (they don't support AXUIElement)
            AXUIElementRef app = AXUIElementCreateApplication(pid);
            AXUIElementRef frontMostWindow;
            AXError err = AXUIElementCopyAttributeValue(
                app, kAXFocusedWindowAttribute, (CFTypeRef *)&frontMostWindow);
            CFRelease(app);
            if (err != kAXErrorSuccess)
                continue;
        }

        CGRect rect;
        CGRectMakeWithDictionaryRepresentation(
            (CFDictionaryRef)window[(id)kCGWindowBounds], &rect);

        Bounds bounds{(int)rect.origin.x, (int)rect.origin.y,
                      (int)rect.size.width, (int)rect.size.height};

        if (is_display == false)
        {
            Window window{bounds, std::move(title), pid};
            window_list.add_window(owner, std::move(window));
            got_first = true;
        }
        else
        {
            display_list.add_display(bounds);
        }
    }

    if (window_arr.count > 0)
        CFRelease(window_arr);

    return std::make_pair(window_list, display_list); // RVO
}

bool apply_bounds(const Window &window, Bounds bounds) // does not alter window
{
    bool success(false);

    AXUIElementRef app = AXUIElementCreateApplication(window.pid);

    NSArray *window_arr;
    AXUIElementCopyAttributeValues( // 1024 is maximum # of elements
        app, kAXWindowsAttribute, 0, 1024, (CFArrayRef *) &window_arr);
    CFRelease(app);

    // find AXUIElement with matching origin, size, title 
    // (this is the only known way of getting Accessibility API's
    //  window objects from pid/kCGWindowNumber within the documented API)
    for (id element in window_arr)
    {
        AXUIElementRef w = (__bridge AXUIElementRef)element;
        AXValueRef v;

        CGPoint origin;
        AXUIElementCopyAttributeValue(w, kAXPositionAttribute, (CFTypeRef*)&v);
        AXValueGetValue(v, (AXValueType)kAXValueCGPointType, &origin);
        CFRelease(v);

        if ((int)origin.x != window.bounds.x ||
            (int)origin.y != window.bounds.y)
            continue;

        CGSize size;
        AXUIElementCopyAttributeValue(w, kAXSizeAttribute, (CFTypeRef*)&v);
        AXValueGetValue(v, (AXValueType)kAXValueCGSizeType, &size);
        CFRelease(v);

        if ((int)size.width  != window.bounds.w ||
            (int)size.height != window.bounds.h)
            continue;
        
        AXUIElementCopyAttributeValue(w, kAXTitleAttribute, (CFTypeRef*)&v);
        const char * title_c_str = [(__bridge NSString *)v UTF8String];
        STR title(title_c_str != NULL ? title_c_str : "");
        CFRelease(v);

        if (title != window.title)
            continue;

        if (window.bounds.x != bounds.x || window.bounds.y != bounds.y)
        {
            origin.x = (CGFloat)bounds.x;
            origin.y = (CGFloat)bounds.y;
            v = AXValueCreate((AXValueType)kAXValueCGPointType, &origin);
            AXUIElementSetAttributeValue(w, kAXPositionAttribute, v);
            CFRelease(v);
        }

        if (window.bounds.w != bounds.w || window.bounds.h != bounds.h)
        {
            size.width  = (CGFloat)bounds.w;
            size.height = (CGFloat)bounds.h;
            v = AXValueCreate((AXValueType)kAXValueCGSizeType, &size);
            AXUIElementSetAttributeValue(w, kAXSizeAttribute, v);
            CFRelease(v);
        }

        success = true;
        break;
    }

    if (window_arr.count > 0)
        CFRelease(window_arr);
    
    return success;
}

int main(int argc, const char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    auto clean_exit = [&pool](int exit_code) {
        [pool drain];
        return exit_code;
    };

    // obtain access to Accessibility API if not available
    NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
    if (false == AXIsProcessTrustedWithOptions((CFDictionaryRef)options))
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
        if (false == build_lists().first.print(args.substr))
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
            auto win_list = build_lists().first; // lifetime affects pair_ptr
            auto pair_ptr = win_list.find(std::get<0>(b));
            if (pair_ptr != nullptr &&
                std::get<1>(b) < pair_ptr->second.size())
            {
                const auto &win = pair_ptr->second[std::get<1>(b)];
                valid = apply_bounds(win, 
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
        auto win_disp = (args.substr.empty() == true ? build_lists(true) :
                                                       build_lists());
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

        if (true == apply_bounds(win, bounds))
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


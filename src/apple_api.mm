
#include "apple_api.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

namespace apple_api
{

bool enable_accessibility_api()
{
    NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
    return AXIsProcessTrustedWithOptions((CFDictionaryRef)options);
}

std::pair<WindowList, DisplayList> build_lists(bool front_only)
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
        int pid = [window[(id)kCGWindowOwnerPID] intValue];
        
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
            window_list.add_window(bounds, owner, title, pid);
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

bool apply_bounds(const Window &window, Bounds bounds)
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

int get_menu_bar_owner_pid()
{
    for (NSRunningApplication *app in
        [[NSWorkspace sharedWorkspace] runningApplications])
    {
        if ([app activationPolicy] == NSApplicationActivationPolicyRegular
         && [app ownsMenuBar])
            return [app processIdentifier];
    }

    return 0;
}

}

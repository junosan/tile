#include "Bounds.h"

#include <algorithm>
#include <cmath>

int Bounds::overlap_area(Bounds other) const
{
    auto overlap_len = [](int o1, int s1, int o2, int s2) {
        return (o1 < o2 ? std::min(std::max(o1 + s1 - o2, 0), s2) :
                          std::min(std::max(o2 + s2 - o1, 0), s1));
    };
    return overlap_len(x, w, other.x, other.w) *
           overlap_len(y, h, other.y, other.h);
}

void Bounds::attach(Bounds other)
{
    int l = std::min(x, other.x);
    int t = std::min(y, other.y);
    int r = std::max(x + w, other.x + other.w);
    int b = std::max(y + h, other.y + other.h);
    std::tie(x, y, w, h) = std::make_tuple(l, t, r - l, b - t);
}

Bounds Bounds::snap(Bounds in, snap_dir dir) const
{
    switch (dir)
    {
        case snap_dir::l: in.x = x - in.w; break;
        case snap_dir::r: in.x = x + w;    break;
        case snap_dir::t: in.y = y - in.h; break;
        case snap_dir::b: in.y = y + h;    break;
        default: verify(false);
    }
    return in;
}

Bounds Bounds::fit(Bounds in) const
{
    auto fit_1d = [](int o1, int s1, int o2, int s2) {
        o2 -= std::max(o2 + s2 - o1 - s1, 0);
        o2 += std::max(o1 - o2, 0);
        s2  = std::min(s1, s2);
        return std::make_pair(o2, s2);
    };

    std::tie(in.x, in.w) = fit_1d(x, w, in.x, in.w);
    std::tie(in.y, in.h) = fit_1d(y, h, in.y, in.h);

    return in;
}

Bounds Bounds::h_sub(Bounds in, float unit, float l, float r) const
{
    if (l != r || r < 0.f) // see comments in ArgParser::Args
    {
        int rel_orig = std::min(w,
            (l >= 0.f ? static_cast<int>(std::round(l * unit)) : in.x - x));
        int rel_term = std::min(w,
            (r >= 0.f ? static_cast<int>(std::round(r * unit)) : w));
        rel_orig = std::min(rel_orig, rel_term - 1);

        in.x = x + rel_orig;
        in.w = rel_term - rel_orig;
    }

    return in;
}

Bounds Bounds::v_sub(Bounds in, ArgParser::v_cmd v_cmd) const
{
    if (v_cmd != ArgParser::v_cmd::none)
    {
        float unit;
        switch (v_cmd)
        {
            case ArgParser::v_cmd::f: unit = h; break;
            case ArgParser::v_cmd::t:
            case ArgParser::v_cmd::b: unit = h / 2.f; break;
            case ArgParser::v_cmd::y:
            case ArgParser::v_cmd::h:
            case ArgParser::v_cmd::n:
            case ArgParser::v_cmd::j:
            case ArgParser::v_cmd::k: unit = h / 3.f; break;
            default: verify(false);
        }

        float begin;
        switch (v_cmd)
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
        switch (v_cmd)
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

        int rel_orig = std::min(static_cast<int>(std::round(unit * begin)), h);
        int rel_term = std::min(static_cast<int>(std::round(unit * end  )), h);
        rel_orig = std::min(rel_orig, rel_term - 1);

        in.y = y + rel_orig;
        in.h = rel_term - rel_orig;
    }

    return in;
}

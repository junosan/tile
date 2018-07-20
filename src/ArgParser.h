#ifndef ARGPARSER_H_
#define ARGPARSER_H_

#include "common.h"

class ArgParser {
public:
    enum class action { invalid, help, list, undo, snap, move, tile };
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
        long index;

        Args() : action{action::invalid},
                h_cmd{0.f, 0.f}, v_cmd{v_cmd::none}, m_dir{m_dir::none},
                index{0} {}
    };

    Args parse(int argc, const char *argv[]);
    void print_usage();
};

#endif /* ARGPARSER_H_ */

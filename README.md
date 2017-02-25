
# tile

`tile` is a command-line window manipulation tool for macOS.

Features:
- Address apps by case insensitive substring of their names
  (e.g., `s` for *Safari*, `i` for *iTunes*, etc.),
  or, if no name is provided, act on the currently focused window
- Manipulate windows within one display, or send them to other displays
- Horizontal and vertical dimensions are treated differently
  - Horizontal: position and width addressed in terms of `unit_width`
    (defaults to `Terminal.app`'s width at 80 columns without scrollbar)
  - Vertical: position and height set with multiples of halves or thirds of
    display height
- Snap last *n* focused windows together side-by-side with the most recently
  focused one as the pivot
- Integrate with hotkey managers that issue shell commands such as
  [khd](https://github.com/koekeishiya/khd);
  sample config file for *khd* is provided in `readme/.khdrc`, which results
  in the following keybinds (prefix ^⌘):

![keybinds](readme/keybinds.png)

## How to use
- Clone, then `make`
- Copy just-built `tile` to `/usr/local/bin` for access from anywhere
  (optional)
- See `tile --help` for detailed command syntax
- Some examples:
  - `tile`: list open windows
  - `tile 0`: current window, position 0, width 1 (`unit_width`),
              height unchanged
  - `tile 1-`: current window, position 1, width extended to right end,
               height unchanged
  - `tile 1-3f sa`: position 1, width 2, full height, 0-th window of the
                    app whose name starts with `sa` (e.g., *Safari*)
  - `tile -2.5t fi 1`: position same, width extended to position 2.5,
                       top half, 1-th window of app `fi` (e.g., *Finder*)
  - `tile snap`: snap current window to the closest display edge (left/right)
  - `tile snap 3`: snap last 3 focused windows together side-by-side
  - `tile undo`: undo last action
  - `tile [` or `tile ]`: send current window to previous or next display
        (direction is meaningful only if there are 3 or more displays)
- `unit_width` and screen margins can be configured in `$HOME/.tilerc`

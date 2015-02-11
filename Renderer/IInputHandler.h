#pragma once

// Interface that defines methods to handle input
class IInputHandler
{
public:
    // Handles pressed key with `code`.
    // Flags `shift`, `ctrl` and `alt` show whether the corresponding modifier was pressed
    virtual void process_key(unsigned code, bool shift, bool ctrl, bool alt) = 0;
    // Handles mouse dragged to point (x, y) with delta from previous point equal to (dx, dy).
    // Flags `shift` and `ctrl` show whether the corresponding modifier was being pressed.
    virtual void process_mouse_drag(short x, short y, short dx, short dy, bool shift, bool ctrl) = 0;
    // Handles mouse wheel rotated to dw steps at point (x, y).
    // Flags `shift` and `ctrl` show whether the corresponding modifier was being pressed.
    virtual void process_mouse_wheel(short x, short y, short dw, bool shift, bool ctrl) = 0;
};
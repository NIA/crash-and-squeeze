#pragma once
#include <windows.h>
#include "main.h"
#include "IInputHandler.h"
#include "settings.h"

class Window
{
private:
    HWND hwnd;
    WNDCLASSEX window_class;
    int width;
    int height;

    IInputHandler * input_handler;
    struct MouseState {
        bool dragging;
        short prev_x;
        short prev_y;
    } mouse;

    void unregister_class();
public:
    static const int DEFAULT_WINDOW_SIZE = 1000;

    Window(int width, int height);
    void show() const;
    void update() const;
    // Actual processing function (non-static member)
    LRESULT WINAPI MsgProc( HWND, UINT, WPARAM, LPARAM );
    // Wrapper of processing function (static member), that is passed to window class
    static LRESULT WINAPI _MsgProc( HWND, UINT, WPARAM, LPARAM );

    int get_width() { return width; }
    int get_height() { return height; }

    void set_input_handler(IInputHandler * handler) { input_handler = handler; }

    inline operator HWND() { return this->hwnd; }
    inline operator HWND() const { return this->hwnd; }
    ~Window();
};

class ControlsWindowImpl;

class ControlsWindow
{
private:
    ControlsWindowImpl * impl;
public:
    ControlsWindow();

    // Interface that defines a method to get text info
    class ITextInfo
    {
    public:
        virtual tstring get_text_info() const = 0;
    };

    void create(Window & main_window, ISettingsHandler * settings_handler, ITextInfo * text_info);
    void show();

    ~ControlsWindow();
};
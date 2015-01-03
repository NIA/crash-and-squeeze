#pragma once
#include <windows.h>
#include "main.h"

class Window
{
private:
    HWND hwnd;
    WNDCLASSEX window_class;

    void unregister_class();
public:
    static const int DEFAULT_WINDOW_SIZE = 1000;

    Window(int width, int height);
    void show() const;
    void update() const;
    static LRESULT WINAPI MsgProc( HWND, UINT, WPARAM, LPARAM );

    inline operator HWND() { return this->hwnd; }
    inline operator HWND() const { return this->hwnd; }
    ~Window();
};
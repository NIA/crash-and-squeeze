#include "Window.h"
#include <Windowsx.h> // for GET_[XY]_LPARAM
#include <map>

namespace
{
    const TCHAR *WINDOW_CLASS = _T("Crash-And-Squeeze");
    const TCHAR *WINDOW_TITLE = _T("Crash-And-Squeeze Renderer");

    inline bool is_key_pressed(int virtual_key)
    {
        return ::GetAsyncKeyState(virtual_key) < 0;
    }

    std::map<HWND, Window*> hwnd_to_window;
    void set_window_by_hwnd(HWND hwnd, Window * window) { hwnd_to_window[hwnd] = window; }
    Window * get_window_by_hwnd(HWND hwnd) { if (hwnd_to_window.count(hwnd) > 0) return hwnd_to_window[hwnd]; else return nullptr; }
    void remove_window_by_hwnd(HWND hwnd) { hwnd_to_window.erase(hwnd); }

}

Window::Window(int width, int height) :
    width(width), height(height), input_handler(nullptr)
{
    mouse.prev_x = mouse.prev_y = 0;
    mouse.dragging = false;

    ZeroMemory( &window_class, sizeof(window_class) );

    window_class.cbSize = sizeof( WNDCLASSEX );
    window_class.style = CS_CLASSDC;
    window_class.lpfnWndProc = &_MsgProc;
    window_class.hInstance = GetModuleHandle( NULL );
    window_class.lpszClassName = WINDOW_CLASS;

    RegisterClassEx( &window_class );

    hwnd = CreateWindow( WINDOW_CLASS, WINDOW_TITLE,
                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                         NULL, NULL, window_class.hInstance, NULL );
    if( NULL == hwnd )
    {
        unregister_class();
        throw WindowInitError();
    }
    set_window_by_hwnd(hwnd, this);
}

LRESULT WINAPI Window::MsgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch( msg )
    {
        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;
        case WM_KEYDOWN:
            // TODO: is GetAsyncKeyState good way to know WAS the shift pressed when this message was posted (not processed)?
            if (nullptr != input_handler)
            {
                input_handler->process_key( static_cast<unsigned>( wparam ), is_key_pressed(VK_SHIFT),
                                            is_key_pressed(VK_CONTROL), is_key_pressed(VK_MENU) );
                return 0; // processed message
            }
            break;
        case WM_LBUTTONDOWN:
            // start dragging
            mouse.dragging = true;
            mouse.prev_x = GET_X_LPARAM(lparam);
            mouse.prev_y = GET_Y_LPARAM(lparam);
            SetCapture(hwnd);
            return 0; // processed message
        case WM_LBUTTONUP:
            // stop dragging
            mouse.dragging = false;
            ReleaseCapture();
            return 0; // processed message
        case WM_MOUSEMOVE:
            if (mouse.dragging)
            {
                short x = GET_X_LPARAM(lparam);
                short y = GET_Y_LPARAM(lparam);
                bool shift = (wparam & MK_SHIFT) != 0;
                bool ctrl  = (wparam & MK_CONTROL) != 0;
                if (nullptr != input_handler)
                {
                    input_handler->process_mouse_drag(x, y, x - mouse.prev_x, y - mouse.prev_y, shift, ctrl);
                }
                mouse.prev_x = x;
                mouse.prev_y = y;
                return 0; // processed message
            }
            break;
        case WM_MOUSEWHEEL:
            if (nullptr != input_handler)
            {
                input_handler->process_mouse_wheel(GET_X_LPARAM(lparam),
                    GET_Y_LPARAM(lparam),
                    GET_WHEEL_DELTA_WPARAM(wparam),
                    (GET_KEYSTATE_WPARAM(wparam) & MK_SHIFT) != 0,
                    (GET_KEYSTATE_WPARAM(wparam) & MK_CONTROL) != 0);
                return 0; // processed message
            }
            break;
    }

    return DefWindowProc( hwnd, msg, wparam, lparam );
}

LRESULT WINAPI Window::_MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    Window * window = get_window_by_hwnd(hwnd);
    if (nullptr != window)
        return window->MsgProc(hwnd, msg, wparam, lparam);
    else
        return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Window::show() const
{
    ShowWindow( *this, SW_SHOWDEFAULT );
}

void Window::update() const
{
    UpdateWindow( *this );
}

void Window::unregister_class()
{
    UnregisterClass( WINDOW_CLASS, window_class.hInstance );
}

Window::~Window()
{
    remove_window_by_hwnd(hwnd);
    unregister_class();
}


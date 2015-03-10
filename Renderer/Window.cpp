#include "Window.h"
#include <Windowsx.h> // for GET_[XY]_LPARAM
#include <map>
#include "resource.h"
#include "Application.h" // for DEFAULT_CLUSTERS_BY_AXES

using ::CrashAndSqueeze::Math::VECTOR_SIZE;

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
    window_class.hIcon = LoadIcon(window_class.hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));

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

// Enable visual styles
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

ControlsWindow::ControlsWindow() : main_window(nullptr)
{
    // TODO: getting this params from application in a normal way
    params.set_defaults();
    for (int i = 0; i < VECTOR_SIZE; ++i)
        clusters_by_axes[i] = Application::DEFAULT_CLUSTERS_BY_AXES[i];
    show_mode = 0;

    info = 
        _T("Crash-And-Squeeze version 0.9\r\n")
        _T("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n")
        _T("Simulation: ON\r\n")
        _T("Show: Graphical vertices\r\n\r\n")
        _T("Performance: 119 FPS (8.39 ms/frame)\r\n")
        _T("Model:\r\n")
        _T("~~~~~~\r\n")
        _T("19802 low-vertices\r\n")
        _T("19802 high-vertices\r\n")
        _T("24=2x3x4 clusters\r\n\r\n")
        _T("Keyboard controls:\r\n")
        _T("~~~~~~~~~~~~~~~~~~\r\n")
        _T("Enter: hit the model,\r\n")
        _T("I/J/K/L: move hit area (yellow sphere),\r\n")
        _T("Arrows: rotate camera,\r\n")
        _T("+/-, PgUp/PgDn: zoom in/out,\r\n")
        _T("Esc: exit.\r\n\r\n")
        _T("Advanced:\r\n")
        _T("~~~~~~~~~\r\n")
        _T("Tab: switch between current, initial\r\n")
        _T("        and equilibrium state,\r\n")
        _T("Space: pause/continue emulation,\r\n")
        _T("S: emulate one step (when paused),\r\n")
        _T("F: toggle forces on/off,\r\n")
        _T("W: toggle wireframe on/off,\r\n");
}
LRESULT ControlsWindow::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HICON icon = WTL::AtlLoadIconImage(IDI_MAIN_ICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
    SetIcon(icon, TRUE);
    HICON icon_small = WTL::AtlLoadIconImage(IDI_MAIN_ICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
    SetIcon(icon_small, FALSE);

    // TODO: find a way to allow WS_CLIPCHILDREN without whitening group boxes to reduce flicker (maybe use http://www.codeproject.com/Articles/5364/CDialogResize-based-class-with-no-groupbox-flicker ?)
    DlgResize_Init(true, true, 0 /*do not force WS_CLIPCHILDREN to avoid groubox whitening*/ );
    UIAddChildWindowContainer(m_hWnd);

    // Populate "Show mode" combo box
    WTL::CComboBox show_mode_cb = GetDlgItem(IDC_SHOW_MODE);
    for (int i = 0; i < Application::_SHOW_MODES_COUNT; ++i)
        show_mode_cb.AddString(Application::SHOW_MODES_CAPTIONS[i]);

    return TRUE;
}

void ControlsWindow::create(Window & main_window)
{
    this->main_window = main_window;
    Create(main_window);
}

void ControlsWindow::show()
{
    // load initial values
    DoDataExchange(DDX_LOAD);

    // show window...
    ShowWindow(SW_SHOW);

    // ... and position it next to main_window
    RECT mw_rect;
    main_window.GetWindowRect(&mw_rect);
    SetWindowPos(main_window, mw_rect.right, mw_rect.top, -1, -1, SWP_NOSIZE /*ignore -1 size*/);
}

LRESULT ControlsWindow::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // to be filled in future
    return 0;
}

class AboutDlg : public ATL::CDialogImpl<AboutDlg>
{
public:
    enum { IDD = IDD_ABOUT };

    BEGIN_MSG_MAP(AboutDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
        COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CenterWindow(GetParent());
        return TRUE;
    }

    LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        EndDialog(wID);
        return 0;
    }
};

LRESULT ControlsWindow::OnHelpAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    AboutDlg dlg;
    dlg.DoModal();
    return 0;
}

LRESULT ControlsWindow::OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // TODO: do data exchange
    return 0;
}

LRESULT ControlsWindow::OnHide(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ShowWindow(SW_HIDE);
    return 0;
}

LRESULT ControlsWindow::OnQuit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ::DestroyWindow(main_window);
    return 0;
}

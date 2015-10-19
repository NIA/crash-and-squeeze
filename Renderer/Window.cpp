#include "Window.h"
#include <Windowsx.h> // for GET_[XY]_LPARAM
#include <map>
#include "resource.h"
#include "Core/simulation_params.h"

// WTL:
#include <atlbase.h>
#include <atlwin.h> // for CWindow
#define _WTL_NO_AUTOMATIC_NAMESPACE
#include <atlapp.h> // for CAppModule
extern WTL::CAppModule _Module;
#define _ATL_USE_DDX_FLOAT
#include <atlframe.h>// for CUpdateUI
#include <atlmisc.h> // for CString
#define _WTL_USE_CSTRING
#include <atlctrls.h> // for CComboBox, DDX_COMBO_INDEX (and other controls)
#include <atlddx.h>   // for CWinDataExchange, DDX_*

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

class ControlsWindowImpl : public ATL::CDialogImpl<ControlsWindowImpl>,
    public WTL::CDialogResize<ControlsWindowImpl>,
    public WTL::CUpdateUI<ControlsWindowImpl>,
    public WTL::CWinDataExchange<ControlsWindowImpl>
{
private:
    ATL::CWindow main_window;
    ISettingsHandler * settings_handler;
    ControlsWindow::ITextInfo * text_info_handler;

    // DDX variables:
    SimulationSettings sim_settings;
    GlobalSettings  global_settings;
    RenderSettings  render_settings;
    WTL::CString info;
public:
    enum
    {
        IDD = IDD_CONTROLS,

        // Timer settings
        ID_TIMER_TEXT = 42,
        TIMER_TEXT_DELAY = 1000,
    };

    ControlsWindowImpl();

    void create(Window & main_window, ISettingsHandler * settings_handler, ControlsWindow::ITextInfo * text_info);
    void show();
    BEGIN_UPDATE_UI_MAP(ControlsWindowImpl)
        // To be filled in future
    END_UPDATE_UI_MAP()

    BEGIN_MSG_MAP(ControlsWindowImpl)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(ID_HELP_ABOUT, OnHelpAbout)
        COMMAND_ID_HANDLER(IDOK,          OnApply)
        COMMAND_ID_HANDLER(IDC_CANCEL,      OnCancel) // IDC_CANCEL (Cancel button) means "Cancel all changes since last Apply"
        COMMAND_ID_HANDLER(IDC_DEFAULTS,  OnDefaults)
        COMMAND_ID_HANDLER(IDCANCEL,       OnHide)    // IDCCANCEL (Hide button or [x]) means "Hide window"
        COMMAND_ID_HANDLER(ID_FILE_QUIT,  OnQuit)
        CHAIN_MSG_MAP(CDialogResize<ControlsWindowImpl>)
    END_MSG_MAP()

    static const UINT IDC_ED_CLUSTERS[GlobalSettings::AXES_COUNT];

    // TODO: Use for-loops to shorten this map?
    // TODO: handle spinners
    BEGIN_DDX_MAP(ControlsWindowImpl)
        // SimulationSettings
        DDX_FLOAT(IDC_ED_DAMPING,       sim_settings.damping_fraction)
        DDX_FLOAT(IDC_ED_GOAL_SPEED,    sim_settings.goal_speed_fraction)
        DDX_FLOAT(IDC_ED_LINEAR_ELAST,  sim_settings.linear_elasticity_fraction)
        DDX_FLOAT(IDC_ED_YIELD,         sim_settings.yield_threshold)
        DDX_FLOAT(IDC_ED_CREEP,         sim_settings.creep_speed)
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
        DDX_FLOAT(IDC_ED_QX_CREEP,      sim_settings.quadratic_creep_speed)
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
        DDX_FLOAT(IDC_ED_MAX_DEFORM,    sim_settings.max_deformation)
        // GlobalSettings
        for (int i = 0; i < GlobalSettings::AXES_COUNT; ++i)
            DDX_INT(IDC_ED_CLUSTERS[i], global_settings.clusters_by_axes[i]);
        DDX_CHECK(IDC_GPU_UPDATE,       global_settings.update_vertices_on_gpu);
        DDX_CHECK(IDC_QUAD_DEFORM,      global_settings.quadratic_deformation);
        DDX_CHECK(IDC_QUAD_PLAST,       global_settings.quadratic_plasticity);
        DDX_CHECK(IDC_TRANSF_TOTAL,     global_settings.gfx_transform_total);
        // RenderSettings
        DDX_COMBO_INDEX(IDC_SHOW_MODE,  render_settings.show_mode);
        DDX_CHECK(IDC_WIREFRAME,        render_settings.wireframe);
        DDX_CHECK(IDC_NORMALS,          render_settings.show_normals);
        DDX_CHECK(IDC_CLUSTER_PAINT,    render_settings.paint_clusters);
        // ...other:
        DDX_TEXT(IDC_INFO, info);
    END_DDX_MAP()

    // TODO: resize params inputs, not only info
    BEGIN_DLGRESIZE_MAP(ControlsWindowImpl)
        DLGRESIZE_CONTROL(IDC_INFO, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_DEFAULTS, DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    // -- message handlers: --

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

    // -- command handlers

    LRESULT OnHelpAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDefaults(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnHide(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnQuit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    ~ControlsWindowImpl();
};

ControlsWindow::ControlsWindow()
{
    impl = new ControlsWindowImpl;
}

void ControlsWindow::create(Window & main_window, ISettingsHandler * settings_handler, ControlsWindow::ITextInfo * text_info)
{
    impl->create(main_window, settings_handler, text_info);
}

void ControlsWindow::show()
{
    impl->show();
}

ControlsWindow::~ControlsWindow()
{
    delete impl;
}

const UINT ControlsWindowImpl::IDC_ED_CLUSTERS[GlobalSettings::AXES_COUNT] = {IDC_ED_CLUSTERS_X, IDC_ED_CLUSTERS_Y, IDC_ED_CLUSTERS_Z};

ControlsWindowImpl::ControlsWindowImpl()
    : main_window(nullptr), settings_handler(nullptr)
{
    // Temporarily set defaults for all parameters until settings_handler is set up
    sim_settings.set_defaults();
    global_settings.set_defaults();
    render_settings.set_defaults();

    info = _T("Loading...");
}
LRESULT ControlsWindowImpl::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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
    for (int i = 0; i < RenderSettings::_SHOW_MODES_COUNT; ++i)
        show_mode_cb.AddString(RenderSettings::SHOW_MODES_CAPTIONS[i]);
    return TRUE;
}

void ControlsWindowImpl::create(Window & main_window, ISettingsHandler * settings_handler, ControlsWindow::ITextInfo * text_info_handler)
{
    this->main_window = main_window;
    this->settings_handler = settings_handler;
    this->text_info_handler = text_info_handler;

    Create(main_window);

    // load initial values
    this->settings_handler->get_settings(sim_settings, global_settings, render_settings);
    info = this->text_info_handler->get_text_info().c_str();
    DoDataExchange(DDX_LOAD);

    // Set timer for updating text info
    SetTimer(ID_TIMER_TEXT, TIMER_TEXT_DELAY);
}

void ControlsWindowImpl::show()
{
    // show window...
    ShowWindow(SW_SHOW);

    // ... and position it next to main_window
    RECT mw_rect;
    main_window.GetWindowRect(&mw_rect);
    SetWindowPos(main_window, mw_rect.right, mw_rect.top, -1, -1, SWP_NOSIZE /*ignore -1 size*/);

    main_window.SetFocus();
}


LRESULT ControlsWindowImpl::OnTimer(UINT /*uMsg*/, WPARAM timer_id, LPARAM /*lParam*/, BOOL& handled)
{
    if (ID_TIMER_TEXT == timer_id)
    {
        if (nullptr != text_info_handler)
        {
            info = text_info_handler->get_text_info().c_str();
            DoDataExchange(DDX_LOAD, IDC_INFO);
        }
        handled = TRUE;
    }
    else
    {
        handled = FALSE;
    }
    return 0;
}


LRESULT ControlsWindowImpl::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    KillTimer(ID_TIMER_TEXT);
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

LRESULT ControlsWindowImpl::OnHelpAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    AboutDlg dlg;
    dlg.DoModal();
    return 0;
}

LRESULT ControlsWindowImpl::OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // get settings from dialog...
    DoDataExchange(DDX_SAVE);

    // ...set them to application
    settings_handler->set_settings(sim_settings, global_settings, render_settings);
    return 0;
}

LRESULT ControlsWindowImpl::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // get current settings from application...
    settings_handler->get_settings(sim_settings, global_settings, render_settings);

    // ...set them to dialog
    DoDataExchange(DDX_LOAD);
    return 0;
}

LRESULT ControlsWindowImpl::OnDefaults(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // set defaults for all settings...
    sim_settings.set_defaults();
    global_settings.set_defaults();
    render_settings.set_defaults();

    // ...set them to dialog
    DoDataExchange(DDX_LOAD);

    // ...and to application
    settings_handler->set_settings(sim_settings, global_settings, render_settings);
    return 0;
}

LRESULT ControlsWindowImpl::OnHide(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ShowWindow(SW_HIDE);
    return 0;
}

LRESULT ControlsWindowImpl::OnQuit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    DestroyWindow();
    ::DestroyWindow(main_window);
    return 0;
}

ControlsWindowImpl::~ControlsWindowImpl()
{
    if (IsWindow())
        DestroyWindow();
}

#pragma once
#include <windows.h>
#include "main.h"
#include "resource.h"
#include "IInputHandler.h"

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

#include "Core/simulation_params.h"

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

class ControlsWindow : public ATL::CDialogImpl<ControlsWindow>,
                       public WTL::CDialogResize<ControlsWindow>,
                       public WTL::CUpdateUI<ControlsWindow>,
                       public WTL::CWinDataExchange<ControlsWindow>
{
private:
    ATL::CWindow main_window;
    // DDX variables:
    ::CrashAndSqueeze::Core::SimulationParams params;
    int clusters_by_axes[::CrashAndSqueeze::Math::VECTOR_SIZE];
    int show_mode;
    
    WTL::CString info;
public:
    enum { IDD = IDD_CONTROLS };

    ControlsWindow();

    void create(Window & main_window);
    void show();
    BEGIN_UPDATE_UI_MAP(ControlsWindow)
        // To be filled in future
    END_UPDATE_UI_MAP()

    BEGIN_MSG_MAP(ControlsWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(ID_HELP_ABOUT, OnHelpAbout)
        COMMAND_ID_HANDLER(IDOK, OnApply)
        COMMAND_ID_HANDLER(IDCANCEL, OnHide)
        COMMAND_ID_HANDLER(ID_FILE_QUIT, OnQuit)
        CHAIN_MSG_MAP(CDialogResize<ControlsWindow>)
    END_MSG_MAP()

    // Use for-loops to shorten this map?
    BEGIN_DDX_MAP(ControlsWindow)
        DDX_FLOAT(IDC_ED_DAMPING,     params.damping_fraction)
        DDX_FLOAT(IDC_ED_GOAL_SPEED,  params.goal_speed_fraction)
        DDX_FLOAT(IDC_ED_LINEAR_ELAST,params.linear_elasticity_fraction)
        DDX_FLOAT(IDC_ED_YIELD,       params.yield_threshold)
        DDX_FLOAT(IDC_ED_CREEP,       params.creep_speed)
#if CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
        DDX_FLOAT(IDC_ED_QX_CREEP,    params.quadratic_creep_speed)
#endif // CAS_QUADRATIC_EXTENSIONS_ENABLED && CAS_QUADRATIC_PLASTICITY_ENABLED
        DDX_FLOAT(IDC_ED_MAX_DEFORM,  params.max_deformation)
        DDX_INT(IDC_ED_CLUSTERS_X,    clusters_by_axes[0])
        DDX_INT(IDC_ED_CLUSTERS_Y,    clusters_by_axes[1])
        DDX_INT(IDC_ED_CLUSTERS_Z,    clusters_by_axes[2])
        DDX_COMBO_INDEX(IDC_SHOW_MODE,show_mode);
        DDX_TEXT(IDC_INFO, info);
    END_DDX_MAP()

    // TODO: resize params inputs, not only info
    BEGIN_DLGRESIZE_MAP(ControlsWindow)
        DLGRESIZE_CONTROL(IDC_INFO, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_DEFAULTS, DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnHelpAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnHide(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnQuit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
#pragma once
#include <windows.h>
#include "main.h"
#include "IInputHandler.h"

#include <atlbase.h>
#include <atlapp.h>
extern CAppModule _Module;

#include <atlwin.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"

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

class ControlsWindow : public ATL::CDialogImpl<ControlsWindow>, public CUpdateUI<ControlsWindow>
{
private:
    CWindow main_window;
public:
    enum { IDD = IDD_CONTROLS };

    ControlsWindow() : main_window(nullptr) {}

    void create(Window & main_window);
    void show();

    BEGIN_UPDATE_UI_MAP(CMainDlg)
        // To be filled in future
    END_UPDATE_UI_MAP()

    BEGIN_MSG_MAP(CMainDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(ID_HELP_ABOUT, OnHelpAbout)
        COMMAND_ID_HANDLER(IDOK, OnApply)
        COMMAND_ID_HANDLER(IDCANCEL, OnHide)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnHelpAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnApply(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnHide(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
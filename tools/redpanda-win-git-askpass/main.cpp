#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include <stdio.h>
#include <tchar.h>

HINSTANCE hInst;

LRESULT CALLBACK MainDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK TxtPasswordWndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

WNDPROC lpfnTxtPasswordWndProc=NULL;
HWND hMainDlg = NULL;
HWND hwndTxtPassword = NULL;

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE /*hPrevInstance*/,
                   LPTSTR lpCmdLine, int nCmdShow) {
    MSG msg;

    hMainDlg = CreateDialog(hInstance, (LPCTSTR)IDD_MAIN_DIALOG, 0,(DLGPROC)MainDlgProc);
    ShowWindow(hMainDlg, nCmdShow);
    hwndTxtPassword = GetDlgItem(hMainDlg,ID_TXT_PASSWORD);
    lpfnTxtPasswordWndProc = (WNDPROC) SetWindowLongPtr(hwndTxtPassword, GWLP_WNDPROC, (LONG_PTR)TxtPasswordWndProc);
    HWND hwndTxtPrompt = GetDlgItem(hMainDlg,ID_TXT_PROMPT);
    Static_SetText(hwndTxtPrompt, lpCmdLine);
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

//In Subclass Proc
LRESULT CALLBACK TxtPasswordWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_KEYDOWN:
        if (wParam==VK_RETURN) {
            TCHAR s[500+1];
            Edit_GetText(hwndTxtPassword,s,500);
#ifdef _UNICODE
            wprintf(L"%ls", s);
#else
            printf("%s",s);
#endif
            DestroyWindow(hMainDlg);
            return TRUE;
        }
        break;

    }

    return CallWindowProc(lpfnTxtPasswordWndProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/) {
    switch (message) {
    case WM_INITDIALOG :
        return TRUE ;
    case WM_COMMAND :
        switch (LOWORD (wParam)) {
        case IDOK :
        case IDCANCEL :
            DestroyWindow(hDlg);
            return TRUE ;
        }
        break ;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;
    case WM_DESTROY:
        PostQuitMessage(0);
        return TRUE;
    };
    return FALSE;
}

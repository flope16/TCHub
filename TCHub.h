#pragma once

#include <windows.h>
#include "resource.h"

// ============================================================================
// TC Hub - Fenêtre principale
// ============================================================================

class TCHubApp
{
public:
    static HINSTANCE hInst;

    static INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static void Show();
};


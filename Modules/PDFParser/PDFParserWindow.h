#pragma once
#include <windows.h>

// Fenêtre de dialogue du module PDF Parser (Win32)
class PDFParserWindow
{
public:
    // Procédure de la boîte de dialogue
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
};

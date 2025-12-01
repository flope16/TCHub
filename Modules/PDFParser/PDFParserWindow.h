#pragma once
#include <windows.h>
#include <string>

// Fen�tre de dialogue du module PDF Parser (Win32)
class PDFParserWindow
{
public:
    // Proc�dure de la bo�te de dialogue
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // M�thodes internes
    static void OnInitDialog(HWND hDlg);
    static void OnBrowseFile(HWND hDlg);
    static void OnParsePdf(HWND hDlg);
    static void UpdateStatus(HWND hDlg, const std::wstring& message);
    static std::wstring GetSelectedFilePath(HWND hDlg);
    static std::string GetSelectedSupplier(HWND hDlg);
};

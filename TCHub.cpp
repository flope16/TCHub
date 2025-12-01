#include "TCHub.h"
#include "Modules/PDFParser/PDFParserWindow.h"

HINSTANCE TCHubApp::hInst = nullptr;

INT_PTR CALLBACK TCHubApp::MainDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_PDFPARSER:
            DialogBox(
                TCHubApp::hInst,
                MAKEINTRESOURCE(IDD_PDFPARSER_DIALOG),
                hDlg,
                PDFParserWindow::DialogProc   // ← maintenant ça existe bien
            );
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

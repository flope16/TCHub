#include "TCHub.h"
#include <windows.h>

// Point d'entrée de l'application Windows
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Stocker l'instance pour l'utiliser dans les dialogues
    TCHubApp::hInst = hInstance;

    // Lancer la fenêtre principale
    DialogBox(
        hInstance,
        MAKEINTRESOURCE(IDD_MAIN_DIALOG),
        nullptr,
        TCHubApp::MainDlgProc
    );

    return 0;
}

#include "PDFParserWindow.h"
#include "..\..\resource.h"
#include "ParserFactory.h"
#include "XlsxWriter.h"
#include <commdlg.h>
#include <sstream>
#include <filesystem>

// Conversion helper: std::wstring vers std::string
std::string wstring_to_string(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}

// Conversion helper: std::string vers std::wstring
std::wstring string_to_wstring(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size);
    return result;
}

void PDFParserWindow::OnInitDialog(HWND hDlg)
{
    // Remplir le combo box avec les fournisseurs
    HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_SUPPLIER);
    auto suppliers = ParserFactory::getSupportedSuppliers();

    for (const auto& supplier : suppliers)
    {
        std::wstring wSupplier = string_to_wstring(supplier);
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)wSupplier.c_str());
    }

    // S�lectionner le premier par d�faut
    SendMessage(hCombo, CB_SETCURSEL, 0, 0);

    // Initialiser le statut
    UpdateStatus(hDlg, L"Prêt à parser un fichier PDF");
}

void PDFParserWindow::OnBrowseFile(HWND hDlg)
{
    OPENFILENAME ofn = { 0 };
    wchar_t szFile[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Fichiers PDF\0*.pdf\0Tous les fichiers\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        SetDlgItemText(hDlg, IDC_EDIT_FILEPATH, szFile);
        UpdateStatus(hDlg, L"Fichier sélectionné");
    }
}

std::wstring PDFParserWindow::GetSelectedFilePath(HWND hDlg)
{
    wchar_t buffer[MAX_PATH];
    GetDlgItemText(hDlg, IDC_EDIT_FILEPATH, buffer, MAX_PATH);
    return std::wstring(buffer);
}

std::string PDFParserWindow::GetSelectedSupplier(HWND hDlg)
{
    HWND hCombo = GetDlgItem(hDlg, IDC_COMBO_SUPPLIER);
    int index = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);

    if (index == CB_ERR)
        return "Lindab"; // Par défaut

    wchar_t buffer[256];
    SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)buffer);
    return wstring_to_string(std::wstring(buffer));
}

void PDFParserWindow::UpdateStatus(HWND hDlg, const std::wstring& message)
{
    SetDlgItemText(hDlg, IDC_STATIC_STATUS, message.c_str());
}

void PDFParserWindow::OnParsePdf(HWND hDlg)
{
    // R�cup�rer le chemin du fichier
    std::wstring wFilePath = GetSelectedFilePath(hDlg);
    if (wFilePath.empty())
    {
        UpdateStatus(hDlg, L"Erreur : Aucun fichier sélectionné");
        MessageBox(hDlg, L"Veuillez sélectionner un fichier PDF", L"Erreur", MB_OK | MB_ICONERROR);
        return;
    }

    std::string filePath = wstring_to_string(wFilePath);

    // V�rifier que le fichier existe
    if (!std::filesystem::exists(filePath))
    {
        UpdateStatus(hDlg, L"Erreur : Fichier introuvable");
        MessageBox(hDlg, L"Le fichier spécifié n'existe pas", L"Erreur", MB_OK | MB_ICONERROR);
        return;
    }

    UpdateStatus(hDlg, L"Parsing en cours...");

    // R�cup�rer le fournisseur s�lectionn�
    std::string supplierName = GetSelectedSupplier(hDlg);
    Supplier supplier = ParserFactory::supplierFromString(supplierName);

    // Cr�er le parseur appropri�
    auto parser = ParserFactory::createParser(supplier);
    if (!parser)
    {
        UpdateStatus(hDlg, L"Erreur : Parseur non disponible");
        MessageBox(hDlg, L"Impossible de créer le parseur", L"Erreur", MB_OK | MB_ICONERROR);
        return;
    }

    // Parser le fichier
    std::vector<PdfLine> lines;
    try
    {
        lines = parser->parse(filePath);
    }
    catch (...)
    {
        UpdateStatus(hDlg, L"Erreur : Échec du parsing");
        MessageBox(hDlg, L"Une erreur est survenue lors du parsing", L"Erreur", MB_OK | MB_ICONERROR);
        return;
    }

    if (lines.empty())
    {
        UpdateStatus(hDlg, L"Avertissement : Aucune donnée trouvée");
        MessageBox(hDlg, L"Aucune ligne de produit n'a été trouvée dans le PDF", L"Avertissement", MB_OK | MB_ICONWARNING);
        return;
    }

    // G�n�rer le nom du fichier de sortie
    std::filesystem::path pdfPath(filePath);
    std::string outputPath = (pdfPath.parent_path() / (pdfPath.stem().string() + ".xml")).string();

    // �crire le fichier XLSX
    UpdateStatus(hDlg, L"Génération du fichier XLSX...");

    if (!XlsxWriter::writeToXlsx(outputPath, lines))
    {
        UpdateStatus(hDlg, L"Erreur : Échec de l'écriture du fichier");
        MessageBox(hDlg, L"Impossible d'écrire le fichier de sortie", L"Erreur", MB_OK | MB_ICONERROR);
        return;
    }

    // Succ�s
    std::wstringstream ss;
    ss << L"Succès ! " << lines.size() << L" lignes extraites.\nFichier : " << string_to_wstring(outputPath);
    UpdateStatus(hDlg, L"Parsing terminé avec succès");
    MessageBox(hDlg, ss.str().c_str(), L"Succès", MB_OK | MB_ICONINFORMATION);
}

INT_PTR CALLBACK PDFParserWindow::DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        OnInitDialog(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_BROWSE:
            OnBrowseFile(hDlg);
            return TRUE;

        case IDC_BTN_PARSE:
            OnParsePdf(hDlg);
            return TRUE;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

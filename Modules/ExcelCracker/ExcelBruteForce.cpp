#include "ExcelBruteForce.h"
#include <fstream>
#include <windows.h>
#include <sstream>

// NOTE: Pour le déchiffrement complet, nous aurions besoin d'OpenSSL
// ou d'une bibliothèque similaire pour implémenter le déchiffrement Office Open XML

std::string ExcelBruteForce::lastError = "";
bool ExcelBruteForce::stopRequested = false;

std::string ExcelBruteForce::getLastError()
{
    return lastError;
}

void ExcelBruteForce::stop()
{
    stopRequested = true;
    OutputDebugStringA("[ExcelBruteForce] Arrêt demandé\n");
}

bool ExcelBruteForce::isEncrypted(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    // Lire les premiers octets du fichier
    char header[8];
    file.read(header, 8);
    file.close();

    // Un fichier Excel crypté commence généralement par la signature OLE "D0 CF 11 E0 A1 B1 1A E1"
    // plutôt que par "PK" (signature ZIP) pour un fichier .xlsx non crypté

    // Signature OLE (Compound File Binary Format)
    if (header[0] == (char)0xD0 && header[1] == (char)0xCF &&
        header[2] == (char)0x11 && header[3] == (char)0xE0 &&
        header[4] == (char)0xA1 && header[5] == (char)0xB1 &&
        header[6] == (char)0x1A && header[7] == (char)0xE1)
    {
        return true; // Probablement crypté
    }

    // Signature ZIP (fichier .xlsx normal non crypté)
    if (header[0] == 'P' && header[1] == 'K')
    {
        return false; // Non crypté
    }

    // Format inconnu
    return false;
}

bool ExcelBruteForce::testPassword(const std::string& filePath, const std::string& password)
{
    // IMPORTANT: Cette fonction est une implémentation simplifiée
    // Pour une implémentation complète, il faudrait:
    //
    // 1. Lire le fichier OLE (Compound File Binary Format)
    // 2. Extraire l'EncryptionInfo stream
    // 3. Parser les paramètres de chiffrement (algorithme, sel, etc.)
    // 4. Dériver la clé à partir du mot de passe avec les paramètres
    // 5. Tenter de déchiffrer une partie du fichier
    // 6. Vérifier l'intégrité (HMAC)
    //
    // Cela nécessite une bibliothèque de chiffrement comme OpenSSL
    // et une implémentation du format Microsoft Office Encryption

    // Pour l'instant, nous allons utiliser une approche qui tente d'ouvrir
    // le fichier via COM Automation (Excel)

    // NOTE: Cette méthode nécessite qu'Excel soit installé sur le système
    // et utilise l'API COM de Windows

    OutputDebugStringA(("[ExcelBruteForce] Test mot de passe: " + password + "\n").c_str());

    // Initialiser COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comInitialized = SUCCEEDED(hr);

    bool passwordCorrect = false;

    try
    {
        // Créer une instance Excel via COM
        // CLSID pour Excel.Application: {00024500-0000-0000-C000-000000000046}
        CLSID clsid;
        hr = CLSIDFromProgID(L"Excel.Application", &clsid);

        if (SUCCEEDED(hr))
        {
            IDispatch* pExcelApp = nullptr;
            hr = CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void**)&pExcelApp);

            if (SUCCEEDED(hr) && pExcelApp != nullptr)
            {
                // Masquer Excel
                VARIANT varVisible;
                varVisible.vt = VT_BOOL;
                varVisible.boolVal = VARIANT_FALSE;

                DISPID dispidVisible;
                LPOLESTR visibleName = (LPOLESTR)L"Visible";
                pExcelApp->GetIDsOfNames(IID_NULL, &visibleName, 1, LOCALE_USER_DEFAULT, &dispidVisible);

                DISPPARAMS dpVisible = { &varVisible, nullptr, 1, 0 };
                pExcelApp->Invoke(dispidVisible, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dpVisible, nullptr, nullptr, nullptr);

                // Obtenir la collection Workbooks
                DISPID dispidWorkbooks;
                LPOLESTR workbooksName = (LPOLESTR)L"Workbooks";
                pExcelApp->GetIDsOfNames(IID_NULL, &workbooksName, 1, LOCALE_USER_DEFAULT, &dispidWorkbooks);

                DISPPARAMS dpNoArgs = { nullptr, nullptr, 0, 0 };
                VARIANT varWorkbooks;
                VariantInit(&varWorkbooks);
                pExcelApp->Invoke(dispidWorkbooks, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dpNoArgs, &varWorkbooks, nullptr, nullptr);

                if (varWorkbooks.vt == VT_DISPATCH && varWorkbooks.pdispVal != nullptr)
                {
                    IDispatch* pWorkbooks = varWorkbooks.pdispVal;

                    // Tenter d'ouvrir le fichier avec le mot de passe
                    DISPID dispidOpen;
                    LPOLESTR openName = (LPOLESTR)L"Open";
                    pWorkbooks->GetIDsOfNames(IID_NULL, &openName, 1, LOCALE_USER_DEFAULT, &dispidOpen);

                    // Préparer les paramètres
                    VARIANT varFileName, varPassword;
                    VariantInit(&varFileName);
                    VariantInit(&varPassword);

                    // Convertir le chemin en BSTR
                    std::wstring wFilePath(filePath.begin(), filePath.end());
                    varFileName.vt = VT_BSTR;
                    varFileName.bstrVal = SysAllocString(wFilePath.c_str());

                    // Convertir le mot de passe en BSTR
                    std::wstring wPassword(password.begin(), password.end());
                    varPassword.vt = VT_BSTR;
                    varPassword.bstrVal = SysAllocString(wPassword.c_str());

                    // Créer le tableau de paramètres (dans l'ordre inverse pour COM)
                    VARIANT params[15];
                    for (int i = 0; i < 15; i++)
                    {
                        VariantInit(&params[i]);
                        params[i].vt = VT_ERROR;
                        params[i].scode = DISP_E_PARAMNOTFOUND;
                    }

                    params[14] = varFileName;  // Filename (premier paramètre)
                    params[12] = varPassword;  // Password (troisième paramètre)

                    DISPPARAMS dpOpen = { params, nullptr, 15, 0 };
                    VARIANT varResult;
                    EXCEPINFO excepInfo;
                    UINT argErr;
                    VariantInit(&varResult);
                    ZeroMemory(&excepInfo, sizeof(excepInfo));

                    hr = pWorkbooks->Invoke(dispidOpen, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dpOpen, &varResult, &excepInfo, &argErr);

                    if (SUCCEEDED(hr) && varResult.vt == VT_DISPATCH && varResult.pdispVal != nullptr)
                    {
                        // Le fichier a été ouvert avec succès !
                        passwordCorrect = true;
                        OutputDebugStringA(("[ExcelBruteForce] MOT DE PASSE TROUVÉ: " + password + "\n").c_str());

                        // Fermer le classeur sans sauvegarder
                        IDispatch* pWorkbook = varResult.pdispVal;
                        DISPID dispidClose;
                        LPOLESTR closeName = (LPOLESTR)L"Close";
                        pWorkbook->GetIDsOfNames(IID_NULL, &closeName, 1, LOCALE_USER_DEFAULT, &dispidClose);

                        VARIANT varSaveChanges;
                        varSaveChanges.vt = VT_BOOL;
                        varSaveChanges.boolVal = VARIANT_FALSE;

                        DISPPARAMS dpClose = { &varSaveChanges, nullptr, 1, 0 };
                        pWorkbook->Invoke(dispidClose, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dpClose, nullptr, nullptr, nullptr);

                        pWorkbook->Release();
                    }

                    // Nettoyer
                    SysFreeString(varFileName.bstrVal);
                    SysFreeString(varPassword.bstrVal);
                    VariantClear(&varResult);

                    pWorkbooks->Release();
                }

                VariantClear(&varWorkbooks);

                // Quitter Excel
                DISPID dispidQuit;
                LPOLESTR quitName = (LPOLESTR)L"Quit";
                pExcelApp->GetIDsOfNames(IID_NULL, &quitName, 1, LOCALE_USER_DEFAULT, &dispidQuit);
                pExcelApp->Invoke(dispidQuit, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dpNoArgs, nullptr, nullptr, nullptr);

                pExcelApp->Release();
            }
        }
    }
    catch (...)
    {
        OutputDebugStringA("[ExcelBruteForce] Exception lors du test du mot de passe\n");
    }

    if (comInitialized)
    {
        CoUninitialize();
    }

    return passwordCorrect;
}

bool ExcelBruteForce::generateCombinationsRecursive(
    const std::string& charset,
    int length,
    std::string current,
    const std::function<bool(const std::string&)>& callback)
{
    if (stopRequested)
    {
        return false; // Arrêt demandé
    }

    if (length == 0)
    {
        // Appeler le callback avec la combinaison actuelle
        return callback(current);
    }

    for (char c : charset)
    {
        if (!generateCombinationsRecursive(charset, length - 1, current + c, callback))
        {
            return false; // Arrêt demandé ou mot de passe trouvé
        }
    }

    return true;
}

void ExcelBruteForce::generateCombinations(
    const std::string& charset,
    int length,
    const std::function<bool(const std::string&)>& callback)
{
    generateCombinationsRecursive(charset, length, "", callback);
}

std::string ExcelBruteForce::bruteForce(
    const std::string& filePath,
    const Config& config,
    ProgressCallback progressCallback)
{
    stopRequested = false;
    lastError = "";

    OutputDebugStringA(("[ExcelBruteForce] Début du brute-force: " + filePath + "\n").c_str());

    // Vérifier si le fichier est crypté
    if (!isEncrypted(filePath))
    {
        lastError = "Le fichier ne semble pas être crypté. Utilisez ExcelProtectionRemover pour supprimer la protection des feuilles.";
        OutputDebugStringA(("[ExcelBruteForce] " + lastError + "\n").c_str());
        return "";
    }

    int totalAttempts = 0;
    std::string foundPassword = "";

    // Tester toutes les longueurs de mot de passe
    for (int length = config.minLength; length <= config.maxLength && foundPassword.empty(); length++)
    {
        std::stringstream ss;
        ss << "[ExcelBruteForce] Test des mots de passe de longueur " << length << "...\n";
        OutputDebugStringA(ss.str().c_str());

        // Générer toutes les combinaisons pour cette longueur
        generateCombinations(config.charset, length, [&](const std::string& password) -> bool {
            totalAttempts++;

            // Rapporter les progrès
            if (progressCallback != nullptr && totalAttempts % config.progressInterval == 0)
            {
                progressCallback(totalAttempts, password);
            }

            // Tester le mot de passe
            if (testPassword(filePath, password))
            {
                foundPassword = password;
                return false; // Arrêter la génération
            }

            return true; // Continuer
        });

        if (stopRequested)
        {
            lastError = "Processus arrêté par l'utilisateur";
            OutputDebugStringA(("[ExcelBruteForce] " + lastError + "\n").c_str());
            return "";
        }
    }

    if (foundPassword.empty())
    {
        std::stringstream ss;
        ss << "Mot de passe non trouvé après " << totalAttempts << " tentatives";
        lastError = ss.str();
        OutputDebugStringA(("[ExcelBruteForce] " + lastError + "\n").c_str());
    }
    else
    {
        std::stringstream ss;
        ss << "Mot de passe trouvé: " << foundPassword << " après " << totalAttempts << " tentatives";
        OutputDebugStringA(("[ExcelBruteForce] " + ss.str() + "\n").c_str());
    }

    return foundPassword;
}

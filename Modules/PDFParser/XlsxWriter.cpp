#include "XlsxWriter.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <windows.h>

// Fonction helper pour exécuter une commande sans afficher de fenêtre
static int executeCommandSilent(const std::string& command)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // Cacher la fenêtre
    ZeroMemory(&pi, sizeof(pi));

    // Créer une copie modifiable de la commande
    std::string cmdCopy = command;

    // Créer le processus
    if (!CreateProcessA(
        NULL,                   // Nom de l'application
        &cmdCopy[0],           // Ligne de commande (modifiable)
        NULL,                   // Attributs de sécurité du processus
        NULL,                   // Attributs de sécurité du thread
        FALSE,                  // Héritage des handles
        CREATE_NO_WINDOW,       // Drapeaux de création (pas de fenêtre)
        NULL,                   // Environnement
        NULL,                   // Répertoire courant
        &si,                    // Informations de démarrage
        &pi))                   // Informations du processus
    {
        return -1;  // Échec
    }

    // Attendre que le processus se termine
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Récupérer le code de sortie
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // Fermer les handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
}

std::string XlsxWriter::escapeXml(const std::string& str)
{
    std::string result;
    result.reserve(str.size());

    for (char c : str)
    {
        switch (c)
        {
        case '&':  result += "&amp;"; break;
        case '<':  result += "&lt;"; break;
        case '>':  result += "&gt;"; break;
        case '"':  result += "&quot;"; break;
        case '\'': result += "&apos;"; break;
        default:   result += c; break;
        }
    }

    return result;
}

std::string XlsxWriter::generateContentTypesXml()
{
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
<Default Extension="xml" ContentType="application/xml"/>
<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>
<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
<Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>
<Override PartName="/xl/sharedStrings.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml"/>
</Types>)";
}

std::string XlsxWriter::generateRelsXml()
{
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)";
}

std::string XlsxWriter::generateWorkbookXml()
{
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
<sheets>
<sheet name="Données" sheetId="1" r:id="rId1"/>
</sheets>
</workbook>)";
}

std::string XlsxWriter::generateWorkbookRelsXml()
{
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>
<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings" Target="sharedStrings.xml"/>
</Relationships>)";
}

std::string XlsxWriter::generateStylesXml()
{
    return R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
<numFmts count="1">
<numFmt numFmtId="164" formatCode="#,##0.00"/>
</numFmts>
<fonts count="2">
<font><sz val="11"/><name val="Calibri"/></font>
<font><b/><sz val="11"/><name val="Calibri"/><color rgb="FFFFFFFF"/></font>
</fonts>
<fills count="2">
<fill><patternFill patternType="none"/></fill>
<fill><patternFill patternType="solid"><fgColor rgb="FF4472C4"/></patternFill></fill>
</fills>
<borders count="1">
<border><left/><right/><top/><bottom/><diagonal/></border>
</borders>
<cellXfs count="3">
<xf numFmtId="0" fontId="0" fillId="0" borderId="0"/>
<xf numFmtId="0" fontId="1" fillId="1" borderId="0" applyFont="1" applyFill="1"/>
<xf numFmtId="164" fontId="0" fillId="0" borderId="0" applyNumberFormat="1"/>
</cellXfs>
</styleSheet>)";
}

std::string XlsxWriter::generateSharedStringsXml(const std::vector<PdfLine>& lines)
{
    std::ostringstream oss;
    oss << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<sst xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" count=")" << (lines.size() * 2 + 4) << R"(" uniqueCount=")" << (lines.size() * 2 + 4) << R"(">
<si><t>Référence</t></si>
<si><t>Désignation</t></si>
<si><t>Quantité</t></si>
<si><t>Prix HT</t></si>
)";

    for (const auto& line : lines)
    {
        oss << "<si><t>" << escapeXml(line.reference) << "</t></si>\n";
        oss << "<si><t>" << escapeXml(line.designation) << "</t></si>\n";
    }

    oss << "</sst>";
    return oss.str();
}

std::string XlsxWriter::generateSheetXml(const std::vector<PdfLine>& lines)
{
    std::ostringstream oss;
    oss << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
<sheetData>
)";

    // En-têtes (row 1)
    oss << R"(<row r="1">
<c r="A1" s="1" t="s"><v>0</v></c>
<c r="B1" s="1" t="s"><v>1</v></c>
<c r="C1" s="1" t="s"><v>2</v></c>
<c r="D1" s="1" t="s"><v>3</v></c>
</row>
)";

    // Données
    int rowIdx = 2;
    int strIdx = 4;
    for (const auto& line : lines)
    {
        oss << "<row r=\"" << rowIdx << "\">\n";
        oss << "<c r=\"A" << rowIdx << "\" t=\"s\"><v>" << strIdx << "</v></c>\n";  // Référence
        oss << "<c r=\"B" << rowIdx << "\" t=\"s\"><v>" << (strIdx + 1) << "</v></c>\n";  // Désignation
        oss << "<c r=\"C" << rowIdx << "\" s=\"2\"><v>" << std::fixed << std::setprecision(2) << line.quantite << "</v></c>\n";  // Quantité
        oss << "<c r=\"D" << rowIdx << "\" s=\"2\"><v>" << std::fixed << std::setprecision(2) << line.prixHT << "</v></c>\n";  // Prix HT
        oss << "</row>\n";
        rowIdx++;
        strIdx += 2;
    }

    oss << R"(</sheetData>
</worksheet>)";
    return oss.str();
}

bool XlsxWriter::writeToXlsx(const std::string& outputPath, const std::vector<PdfLine>& lines)
{
    try
    {
        // Debug: log de démarrage
        std::string debugMsg = "[XlsxWriter] Début de génération XLSX : " + outputPath + "\n";
        OutputDebugStringA(debugMsg.c_str());

        // Créer un dossier temporaire
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / ("xlsx_" + std::to_string(std::time(nullptr)));

        OutputDebugStringA(("[XlsxWriter] Dossier temporaire : " + tempDir.string() + "\n").c_str());

        std::filesystem::create_directories(tempDir);
        std::filesystem::create_directories(tempDir / "_rels");
        std::filesystem::create_directories(tempDir / "xl");
        std::filesystem::create_directories(tempDir / "xl" / "_rels");
        std::filesystem::create_directories(tempDir / "xl" / "worksheets");

        OutputDebugStringA("[XlsxWriter] Dossiers créés\n");

        // Écrire tous les fichiers XML
        auto writeFile = [](const std::filesystem::path& path, const std::string& content) {
            std::ofstream file(path, std::ios::binary);
            if (!file.is_open()) return false;
            file << content;
            file.close();
            return true;
        };

        if (!writeFile(tempDir / "[Content_Types].xml", generateContentTypesXml()))
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Échec écriture [Content_Types].xml\n");
            return false;
        }
        if (!writeFile(tempDir / "_rels" / ".rels", generateRelsXml()))
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Échec écriture _rels/.rels\n");
            return false;
        }
        if (!writeFile(tempDir / "xl" / "workbook.xml", generateWorkbookXml()))
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Échec écriture workbook.xml\n");
            return false;
        }
        if (!writeFile(tempDir / "xl" / "_rels" / "workbook.xml.rels", generateWorkbookRelsXml()))
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Échec écriture workbook.xml.rels\n");
            return false;
        }
        if (!writeFile(tempDir / "xl" / "styles.xml", generateStylesXml()))
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Échec écriture styles.xml\n");
            return false;
        }
        if (!writeFile(tempDir / "xl" / "sharedStrings.xml", generateSharedStringsXml(lines)))
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Échec écriture sharedStrings.xml\n");
            return false;
        }
        if (!writeFile(tempDir / "xl" / "worksheets" / "sheet1.xml", generateSheetXml(lines)))
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Échec écriture sheet1.xml\n");
            return false;
        }

        OutputDebugStringA("[XlsxWriter] Tous les fichiers XML écrits\n");

        // Créer le ZIP avec PowerShell (Windows uniquement)
        std::filesystem::path absOutputPath = std::filesystem::absolute(outputPath);

        // Supprimer le fichier de sortie s'il existe déjà
        if (std::filesystem::exists(absOutputPath))
        {
            OutputDebugStringA("[XlsxWriter] Suppression du fichier existant\n");
            std::filesystem::remove(absOutputPath);
        }

        // Construire la commande PowerShell avec échappement correct
        std::string tempDirStr = tempDir.string();
        std::string outputPathStr = absOutputPath.string();

        // Remplacer les backslashes par des forward slashes pour PowerShell
        std::replace(tempDirStr.begin(), tempDirStr.end(), '\\', '/');
        std::replace(outputPathStr.begin(), outputPathStr.end(), '\\', '/');

        std::string powershellCmd = "powershell -NoProfile -ExecutionPolicy Bypass -Command \"Compress-Archive -Path '" +
            tempDirStr + "/*' -DestinationPath '" +
            outputPathStr + "' -Force\"";

        OutputDebugStringA(("[XlsxWriter] Commande ZIP : " + powershellCmd + "\n").c_str());

        // Exécuter PowerShell sans afficher de fenêtre
        int result = executeCommandSilent(powershellCmd);

        OutputDebugStringA(("[XlsxWriter] Résultat ZIP : " + std::to_string(result) + "\n").c_str());

        // Vérifier si le fichier a bien été créé
        bool success = (result == 0) && std::filesystem::exists(absOutputPath);

        if (!success)
        {
            OutputDebugStringA("[XlsxWriter] ERREUR: Fichier ZIP non créé, utilisation du fallback SpreadsheetML\n");

            // FALLBACK: Utiliser le format SpreadsheetML (Excel 2003)
            // Ce format est plus simple et ne nécessite pas de ZIP
            // IMPORTANT: On change l'extension en .xml pour que Excel accepte le fichier
            std::filesystem::path xmlOutputPath = absOutputPath;
            xmlOutputPath.replace_extension(".xml");

            try
            {
                std::string xmlContent = generateSpreadsheetML(lines);
                std::ofstream file(xmlOutputPath, std::ios::binary);
                if (file.is_open())
                {
                    file << xmlContent;
                    file.close();
                    success = true;
                    OutputDebugStringA(("[XlsxWriter] Fallback SpreadsheetML réussi : " + xmlOutputPath.string() + "\n").c_str());
                }
                else
                {
                    OutputDebugStringA("[XlsxWriter] ERREUR: Impossible d'écrire le fichier de fallback\n");
                }
            }
            catch (const std::exception& e)
            {
                OutputDebugStringA(("[XlsxWriter] ERREUR fallback: " + std::string(e.what()) + "\n").c_str());
            }
        }
        else
        {
            OutputDebugStringA("[XlsxWriter] Fichier ZIP créé avec succès\n");
        }

        // Nettoyer le dossier temporaire
        try
        {
            std::filesystem::remove_all(tempDir);
            OutputDebugStringA("[XlsxWriter] Dossier temporaire supprimé\n");
        }
        catch (...)
        {
            OutputDebugStringA("[XlsxWriter] ATTENTION: Échec suppression dossier temporaire\n");
        }

        return success;
    }
    catch (const std::exception& e)
    {
        std::string errorMsg = "[XlsxWriter] EXCEPTION: " + std::string(e.what()) + "\n";
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }
    catch (...)
    {
        OutputDebugStringA("[XlsxWriter] EXCEPTION inconnue\n");
        return false;
    }
}

std::string XlsxWriter::generateSpreadsheetML(const std::vector<PdfLine>& lines)
{
    std::ostringstream oss;

    // En-tête XML pour Excel (format SpreadsheetML / Office 2003)
    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    oss << "<?mso-application progid=\"Excel.Sheet\"?>\n";
    oss << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"\n";
    oss << " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\n";

    // Styles
    oss << " <Styles>\n";
    oss << "  <Style ss:ID=\"Header\">\n";
    oss << "   <Font ss:Bold=\"1\"/>\n";
    oss << "   <Interior ss:Color=\"#4472C4\" ss:Pattern=\"Solid\"/>\n";
    oss << "   <Font ss:Color=\"#FFFFFF\"/>\n";
    oss << "  </Style>\n";
    oss << "  <Style ss:ID=\"Currency\">\n";
    oss << "   <NumberFormat ss:Format=\"#,##0.00\"/>\n";
    oss << "  </Style>\n";
    oss << " </Styles>\n";

    // Feuille de calcul
    oss << " <Worksheet ss:Name=\"Données\">\n";
    oss << "  <Table>\n";

    // En-têtes
    oss << "   <Row>\n";
    oss << "    <Cell ss:StyleID=\"Header\"><Data ss:Type=\"String\">Référence</Data></Cell>\n";
    oss << "    <Cell ss:StyleID=\"Header\"><Data ss:Type=\"String\">Désignation</Data></Cell>\n";
    oss << "    <Cell ss:StyleID=\"Header\"><Data ss:Type=\"String\">Quantité</Data></Cell>\n";
    oss << "    <Cell ss:StyleID=\"Header\"><Data ss:Type=\"String\">Prix HT</Data></Cell>\n";
    oss << "   </Row>\n";

    // Données
    for (const auto& line : lines)
    {
        oss << "   <Row>\n";
        oss << "    <Cell><Data ss:Type=\"String\">" << escapeXml(line.reference) << "</Data></Cell>\n";
        oss << "    <Cell><Data ss:Type=\"String\">" << escapeXml(line.designation) << "</Data></Cell>\n";
        oss << "    <Cell ss:StyleID=\"Currency\"><Data ss:Type=\"Number\">"
            << std::fixed << std::setprecision(2) << line.quantite << "</Data></Cell>\n";
        oss << "    <Cell ss:StyleID=\"Currency\"><Data ss:Type=\"Number\">"
            << std::fixed << std::setprecision(2) << line.prixHT << "</Data></Cell>\n";
        oss << "   </Row>\n";
    }

    oss << "  </Table>\n";
    oss << " </Worksheet>\n";
    oss << "</Workbook>\n";

    return oss.str();
}

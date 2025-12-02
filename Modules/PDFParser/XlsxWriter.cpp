#include "XlsxWriter.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>

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
        // Créer un dossier temporaire
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / ("xlsx_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(tempDir);
        std::filesystem::create_directories(tempDir / "_rels");
        std::filesystem::create_directories(tempDir / "xl");
        std::filesystem::create_directories(tempDir / "xl" / "_rels");
        std::filesystem::create_directories(tempDir / "xl" / "worksheets");

        // Écrire tous les fichiers XML
        auto writeFile = [](const std::filesystem::path& path, const std::string& content) {
            std::ofstream file(path, std::ios::binary);
            if (!file.is_open()) return false;
            file << content;
            file.close();
            return true;
        };

        if (!writeFile(tempDir / "[Content_Types].xml", generateContentTypesXml())) return false;
        if (!writeFile(tempDir / "_rels" / ".rels", generateRelsXml())) return false;
        if (!writeFile(tempDir / "xl" / "workbook.xml", generateWorkbookXml())) return false;
        if (!writeFile(tempDir / "xl" / "_rels" / "workbook.xml.rels", generateWorkbookRelsXml())) return false;
        if (!writeFile(tempDir / "xl" / "styles.xml", generateStylesXml())) return false;
        if (!writeFile(tempDir / "xl" / "sharedStrings.xml", generateSharedStringsXml(lines))) return false;
        if (!writeFile(tempDir / "xl" / "worksheets" / "sheet1.xml", generateSheetXml(lines))) return false;

        // Créer le ZIP avec PowerShell (Windows uniquement)
        std::filesystem::path absOutputPath = std::filesystem::absolute(outputPath);
        std::string powershellCmd = "powershell -Command \"Compress-Archive -Path '" +
            tempDir.string() + "\\*' -DestinationPath '" +
            absOutputPath.string() + "' -Force\"";

        int result = system(powershellCmd.c_str());

        // Nettoyer le dossier temporaire
        std::filesystem::remove_all(tempDir);

        return result == 0;
    }
    catch (...)
    {
        return false;
    }
}

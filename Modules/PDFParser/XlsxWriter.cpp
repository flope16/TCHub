#include "XlsxWriter.h"
#include <fstream>
#include <sstream>
#include <iomanip>

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

std::string XlsxWriter::generateXlsxContent(const std::vector<PdfLine>& lines)
{
    std::ostringstream oss;

    // En-tête XML pour Excel
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

bool XlsxWriter::writeToXlsx(const std::string& outputPath, const std::vector<PdfLine>& lines)
{
    try
    {
        std::string content = generateXlsxContent(lines);

        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open())
            return false;

        file << content;
        file.close();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

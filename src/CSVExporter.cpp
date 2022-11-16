#include "CSVExporter.h"

#include <fstream>

#include "CustomTextState.h"
#include "pdf/utils/tools.h"


using std::string;
using std::fstream;

CCSVExporter::CCSVExporter()
{
}


CCSVExporter::~CCSVExporter()
{
}

static inline string fix_text_for_csv(string text)
{
    size_t off = 0;
    auto it = text.find('\"', off);
    while (it != string::npos) {
        text.insert(it, 1, '\"');
        off = it + 2;
        it = text.find('\"', off);
    }
    return text;
}

bool CCSVExporter::ToFile(const std::string& Filename, const std::vector<std::string>& state)
{
    std::string output;
    for (auto& line : state) {
        output += "\"" + line + "\"\n";
    }

    bool rc = true;
    try {
        fstream test(Filename, std::ios::out);
        test.write(output.data(), static_cast<std::streamsize>(output.size()));
    } catch (std::exception&) {
        rc = false;
    }

    return rc;
}

bool CCSVExporter::ToFile(const std::string& Filename, const CCustomTextState& state)
{
    std::string output;
//    int tbl_i = 0;

    output = "\"Trade\",\"Coverage\",\"Item #\",";
    for (auto& header_col : state.tables.front().columns) {
        output += "\"" + fix_text_for_csv(header_col.name) + "\",";
    }
    output += "\n";
    for (auto& tbl : state.tables) {
        const size_t ciDescription = tbl.GetColumn("DESCRIPTION", "Description");
        for (size_t row = 0; row < tbl.rows.size(); row++) {
            output += "\"" + fix_text_for_csv(tbl.trade) + "\",\"" + fix_text_for_csv(tbl.coverage) + "\",";
            string desc = tbl.rows[row][ciDescription];
            string item_no;
            auto item_no_token = desc.find(".");
            if (item_no_token != string::npos) {
                item_no = desc.substr(0, item_no_token);
                desc = desc.substr(item_no_token + 1);
            }
            output += "\"" + fix_text_for_csv(item_no) + "\",";
            for (size_t col = 0; col < tbl.rows[row].size(); col++) {
                if (col == ciDescription) {
                    output += "\"" + fix_text_for_csv(trim_all(desc)) + "\",";
                } else {
                    output += "\"" + fix_text_for_csv(tbl.rows[row][col]) + "\",";
                }
            }
            output += "\n";
        }
        output += "\n\n";
    }

    bool rc = true;
    try {
        fstream test(Filename, std::ios::out);
        test.write(output.data(), static_cast<std::streamsize>(output.size()));
    } catch (std::exception&) {
        rc = false;
    }

    return rc;
}

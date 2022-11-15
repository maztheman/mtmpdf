#include "ExcelExporter.h"

#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "pdf/utils/tools.h"

#include "CustomTextState.h"
#include "CustomState2.h"


using std::string;
using std::fstream;
using std::vector;
using std::stringstream;

static inline bool is_kinda_number(const std::string& value)
{
    const char* first = &value[0];
    const char* last = &value[value.size() - 1];
    
    for (; first != last; ++first) {
        if (*first == ' ') {
            return false;
        } else if (*first >= 'A' && *first <= 'B') {
            return false;
        } else if (*first >= '0' && *first <= '9') {
        } else if (*first == '.') {
        }
    }
    return true;
}


CExcelExporter::CExcelExporter()
{
}


CExcelExporter::~CExcelExporter()
{
}


bool CExcelExporter::ToFile(const std::string& Filename, const CCustomState2& state)
{
    auto workbook  = workbook_new(Filename.c_str());
    auto sheet = workbook_add_worksheet(workbook, "sheet1");

    //sheet->defaultColwidth(16);
    uint16_t iCol = 0;
    uint16_t iRow = 0;
    vector<uint16_t> col_widths;

    //auto header_font = book.font("header");
    //header_font->SetBoldStyle(xlslib_core::BOLDNESS_BOLD);
    //header_font->SetHeight(200);

    for (auto& header_col : state.tables.front().columns) {
        col_widths.push_back(static_cast<uint16_t>(header_col.name.size() & 0xFFFF));
        worksheet_write_string(sheet, iRow, iCol++, header_col.name.c_str(), NULL);
        //cell->halign(xlslib_core::HALIGN_CENTER);
        //cell->font(header_font);
    }

    std::vector<int> RowHeight;

    int iMaxCol = iCol;
    
    iRow++;
    RowHeight.push_back(256);

    auto& tbl = state.tables[0];
    for (size_t row = 0; row < tbl.rows.size(); row++) {
        iCol = 0;
        int MaxLineCount = 1;
        for (size_t col = 0; col < tbl.rows[row].size(); col++) {

            stringstream ss(tbl.rows[row][col]);
            string line;
            while (std::getline(ss, line)) {
                col_widths[iCol] = (std::max)(col_widths[iCol], (uint16_t)line.size());
            }
            worksheet_write_string(sheet, iRow, iCol++, tbl.rows[row][col].c_str(), NULL);
            //sheet->label(iRow, iCol++, tbl.rows[row][col]);
            auto cnt_line = std::count(tbl.rows[row][col].begin(), tbl.rows[row][col].end(), '\n') + 1;
            MaxLineCount = (std::max)(MaxLineCount, (int)cnt_line);
        }
        iRow++;
        RowHeight.push_back(256 * MaxLineCount);
    }

    for (int i = 0; i < iMaxCol; i++) {
        auto width = col_widths[i];
        //state.tables.front().columns[i];
        //sheet->colwidth(i, width * 264);
    }

    for (int row = 0; row < iRow; row++) {
        if (RowHeight[row] > 256) {
            //sheet->rowheight(row, static_cast<unsigned16_t>(RowHeight[row] + 15));
            //sheet->rangegroup(row, 0, row, iMaxCol)->valign(xlslib_core::VALIGN_BOTTOM);
        }
    }

    workbook_close(workbook);

    //book.Dump(Filename);

    return true;
}

bool CExcelExporter::ToFile(const std::string& Filename, const CCustomTextState& state)
{
    auto workbook  = workbook_new(Filename.c_str());
    auto sheet = workbook_add_worksheet(workbook, "sheet1");

    bool rc = true;
    //xlslib_core::workbook book;
    //auto sheet = book.sheet("sheet1");
    //sheet->defaultColwidth(16);
    int iCol = 0;
    int iRow = 0;
    vector<uint16_t> col_widths;

    col_widths.push_back(5);
    worksheet_write_string(sheet, iRow, iCol++, "Trade", NULL);
    
    col_widths.push_back(8); 
    worksheet_write_string(sheet, iRow, iCol++, "Coverage", NULL);
    
    col_widths.push_back(6); 
    worksheet_write_string(sheet, iRow, iCol++, "Item #", NULL);

    if (state.tables.empty()) {
        return false;
    }

    for (auto& header_col : state.tables.front().columns) {
        col_widths.push_back(static_cast<uint16_t>(header_col.name.size() & 0xFFFF));
        worksheet_write_string(sheet, iRow, iCol++,  header_col.name.c_str(), NULL);
    }

    std::vector<int> RowHeight;

    int iMaxCol = iCol;
    
    iRow++;
    RowHeight.push_back(256);

    for (auto& tbl : state.tables) {
        const size_t ciDescription = tbl.GetColumn("DESCRIPTION", "Description");
        for (size_t row = 0; row < tbl.rows.size(); row++) {
            iCol = 0;
            col_widths[iCol] = (std::max)(col_widths[iCol], (uint16_t)tbl.trade.size());
            worksheet_write_string(sheet, iRow, iCol++,  tbl.trade.c_str(), NULL);
            col_widths[iCol] = (std::max)(col_widths[iCol], (uint16_t)tbl.coverage.size());
            worksheet_write_string(sheet, iRow, iCol++,  tbl.coverage.c_str(), NULL);
            string desc;
            if (ciDescription != ~0) {
                desc = tbl.rows[row][ciDescription];
                string item_no;
                auto item_no_token = desc.find(".");
                if (item_no_token != string::npos) {
                    item_no = desc.substr(0, item_no_token);
                    if (is_number(item_no) || is_kinda_number(item_no)) {
                        desc = desc.substr(item_no_token + 1);
                    } else {
                        item_no = "";
                    }
                }
                col_widths[iCol] = (std::max)(col_widths[iCol], (uint16_t)item_no.size());
                worksheet_write_string(sheet, iRow, iCol++,  item_no.c_str(), NULL);
                auto cnt_line = std::count(desc.begin(), desc.end(), '\n') + 1;
                RowHeight.push_back(static_cast<int>(256 * cnt_line));
            } else {
                iCol++;
            }
            for (size_t col = 0; col < tbl.rows[row].size(); col++) {
                if (col == ciDescription) {
                    col_widths[iCol] = 50;
                    worksheet_write_string(sheet, iRow, iCol++,  trim_all(desc).c_str(), NULL);
                } else {
                    col_widths[iCol] = (std::max)(col_widths[iCol], (uint16_t)tbl.rows[row][col].size());
                    worksheet_write_string(sheet, iRow, iCol++,  tbl.rows[row][col].c_str(), NULL);
                }
            }
            iRow++;
        }

        iRow += 2;
        RowHeight.push_back(256);
        RowHeight.push_back(256);
    }
    
    for (int i = 0; i < iMaxCol; i++) {
        auto width = col_widths[i];
        //sheet->colwidth(i, width * 264);
    }

    for (int row = 0; row < iRow; row++) {
        if (RowHeight[row] > 256) {
            //sheet->rowheight(row, static_cast<unsigned16_t>(RowHeight[row] + 15));
            //sheet->rangegroup(row, 0, row, iMaxCol)->valign(xlslib_core::VALIGN_TOP);
        }
    }

    workbook_close(workbook);

    return rc;
}

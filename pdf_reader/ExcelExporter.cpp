#include "stdafx.h"
#include "ExcelExporter.h"

#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "CustomTextState.h"
#include "CustomState2.h"
#include "tools.h"


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
    bool rc = true;
    xlslib_core::workbook book;
    auto sheet = book.sheet("sheet1");
    sheet->defaultColwidth(16);
    int iCol = 0;
    int iRow = 0;
    vector<uint16_t> col_widths;

    auto header_font = book.font("header");
    header_font->SetBoldStyle(xlslib_core::BOLDNESS_BOLD);
    header_font->SetHeight(200);

    for (auto& header_col : state.tables.front().columns) {
        col_widths.push_back(header_col.name.size());
        auto cell = sheet->label(iRow, iCol++, header_col.name);
        cell->halign(xlslib_core::HALIGN_CENTER);
        cell->font(header_font);
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
                col_widths[iCol] = max(col_widths[iCol], line.size());
            }
            auto the_cell = sheet->label(iRow, iCol++, tbl.rows[row][col]);
            auto cnt_line = std::count(tbl.rows[row][col].begin(), tbl.rows[row][col].end(), '\n') + 1;
            MaxLineCount = max(MaxLineCount, (int)cnt_line);
        }
        iRow++;
        RowHeight.push_back(256 * MaxLineCount);
    }

    for (int i = 0; i < iMaxCol; i++) {
        auto width = col_widths[i];
        auto& my_col = state.tables.front().columns[i];
        sheet->colwidth(i, width * 264);
    }

    for (int row = 0; row < iRow; row++) {
        if (RowHeight[row] > 256) {
            sheet->rowheight(row, RowHeight[row] + 15);
            sheet->rangegroup(row, 0, row, iMaxCol)->valign(xlslib_core::VALIGN_BOTTOM);
        }
    }

    book.Dump(Filename);

    return true;
}

bool CExcelExporter::ToFile(const std::string& Filename, const CCustomTextState& state)
{
    bool rc = true;
    xlslib_core::workbook book;
    auto sheet = book.sheet("sheet1");
    sheet->defaultColwidth(16);
    int iCol = 0;
    int iRow = 0;
    vector<uint16_t> col_widths;

    col_widths.push_back(5);
    sheet->label(iRow, iCol++, "Trade");
    
    col_widths.push_back(8); 
    sheet->label(iRow, iCol++, "Coverage");
    
    col_widths.push_back(6); 
    sheet->label(iRow, iCol++, "Item #");

    if (state.tables.empty()) {
        return false;
    }

    for (auto& header_col : state.tables.front().columns) {
        col_widths.push_back(header_col.name.size());
        sheet->label(iRow, iCol++, header_col.name);
    }

    auto fnt = book.font(0);

    std::vector<int> RowHeight;

    int iMaxCol = iCol;
    
    iRow++;
    RowHeight.push_back(256);

    for (auto& tbl : state.tables) {
        const size_t ciDescription = tbl.GetColumn("DESCRIPTION", "Description");
        for (size_t row = 0; row < tbl.rows.size(); row++) {
            iCol = 0;
            col_widths[iCol] = max(col_widths[iCol], tbl.trade.size());
            sheet->label(iRow, iCol++, tbl.trade);
            col_widths[iCol] = max(col_widths[iCol], tbl.coverage.size());
            sheet->label(iRow, iCol++, tbl.coverage);
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
                col_widths[iCol] = max(col_widths[iCol], item_no.size());
                sheet->label(iRow, iCol++, item_no);
                auto cnt_line = std::count(desc.begin(), desc.end(), '\n') + 1;
                RowHeight.push_back(256 * cnt_line);
            } else {
                iCol++;
            }
            for (size_t col = 0; col < tbl.rows[row].size(); col++) {
                if (col == ciDescription) {
                    col_widths[iCol] = 50;
                    sheet->label(iRow, iCol++, trim_all(desc));
                } else {
                    col_widths[iCol] = max(col_widths[iCol], tbl.rows[row][col].size());
                    sheet->label(iRow, iCol++, tbl.rows[row][col]);
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
        sheet->colwidth(i, width * 264);
    }

    for (int row = 0; row < iRow; row++) {
        if (RowHeight[row] > 256) {
            sheet->rowheight(row, RowHeight[row] + 15);
            sheet->rangegroup(row, 0, row, iMaxCol)->valign(xlslib_core::VALIGN_TOP);
        }
    }

    book.Dump(Filename);

    return rc;
}

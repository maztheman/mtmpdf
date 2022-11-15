#pragma once

#include "pdf/pdf.h"


#include <processor/types/Line.h>

struct CustomPdfTable2
{
    std::vector<PdfColumn> columns;
    std::vector<std::vector<std::string>> rows;

    size_t GetColumn(const std::string& sColName, const std::string& sOtherColName = std::string("")) const {
        for (size_t i = 0; i < columns.size(); i++) {
            if (columns[i].name == sColName || columns[i].name == sOtherColName) {
                return i;
            }
        }
        return ~0U;
    }
};

class CCustomState2
{
public:
    CCustomState2();
    ~CCustomState2();

    



    std::vector<std::string> Row;
    std::vector<CustomPdfTable2> tables;
    std::vector<PdfColumn> columns;
    double dHeaderY = -99990.0;
    bool NextRowIsLast = false;
    int iRow = 0;
    std::vector<processor::types::VerticalLine> vertical_lines;
    std::vector<processor::types::HorizontalLine> horizontal_lines;
};


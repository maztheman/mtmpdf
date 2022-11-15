#pragma once

#include "pdf/pdf.h"

#include <processor/types/Line.h>

struct PdfTable
{
    std::string trade;
    std::string coverage;
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

class CCustomTextState
{
    std::vector<const processor::types::VerticalLine*> CollectLinesPerPage(size_t nPage);

public:
    CCustomTextState();
    ~CCustomTextState();

    const processor::types::VerticalLine* FindClosestToTopLeft(double x, double y, size_t nPage);
    const processor::types::VerticalLine* FindClosestToTopRight(double x, double y, size_t nPage);
    

    std::vector<std::string> Row;
    std::string trade;
    std::string locked_trade;
    int iRow = 0;
    std::vector<PdfTable> tables;
    double dHeaderY = -99990.0;
    std::vector<PdfColumn> columns;
    bool NextRowIsLast = false;
    std::vector<processor::types::VerticalLine> vertical_lines;
    std::vector<processor::types::HorizontalLine> horizontal_lines;
    std::vector<std::string> media_boxes;

};


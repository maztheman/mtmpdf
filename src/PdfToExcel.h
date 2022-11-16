#pragma once

#include "CustomTextState.h"
#include "pdf/pdf.h"

#include <processor/types/Line.h>
#include <processor/pdf/types/TextAtLocation.h>

class CPdfDocument;
struct Rectangle;

class CPdfToExcel
{
    CCustomTextState m_State;

public:
    CPdfToExcel();
    ~CPdfToExcel();

    bool ProcessDocument(CPdfDocument* pDocument);
private:

    std::vector<processor::types::HorizontalLine*> CollectHorizontalLines(size_t nPage);
    const processor::types::HorizontalLine* DoesTextHaveAUnderline(const processor::pdf::types::TextAtLocation* text, const std::vector<processor::types::HorizontalLine*>& lines, const struct Rectangle* mb);

    bool ProcessRows(const processor::pdf::types::TextAtLocation& text_data, size_t& iLastCol);
    void SetupColumns(std::vector<const processor::pdf::types::TextAtLocation*>& header_items);
    void TraceBackForTrade(const std::vector<processor::pdf::types::TextAtLocation>& texts, size_t header_index);
    void ProcessAllTextNodes(const std::vector<processor::pdf::types::TextAtLocation>& all_texts);
};


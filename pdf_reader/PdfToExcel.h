#pragma once

#include "CustomTextState.h"
#include "PdfTypes.h"

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

    std::vector<HorizontalLine*> CollectHorizontalLines(size_t nPage);
    const HorizontalLine* DoesTextHaveAUnderline(const TextAtLocation* text, const std::vector<HorizontalLine*>& lines, const struct Rectangle* mb);

    bool ProcessRows(const TextAtLocation& text_data, int& iLastCol);
    void SetupColumns(std::vector<const TextAtLocation*>& header_items);
    void TraceBackForTrade(const std::vector<TextAtLocation>& texts, size_t header_index);
    void ProcessAllTextNodes(const std::vector<TextAtLocation>& all_texts);
};


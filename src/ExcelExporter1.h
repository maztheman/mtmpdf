#pragma once

#include "CustomState2.h"

#include <processor/pdf/types/TextAtLocation.h>
#include <processor/types/Line.h>

class CPdfDocument;

class CExcelExporter1
{
public:
    CExcelExporter1();
    ~CExcelExporter1();

    bool ProcessDocument(CPdfDocument* pDocument);


private:

    void ProcessAllTextNodes(const std::vector<processor::pdf::types::TextAtLocation>& all_texts);
    std::vector<processor::types::HorizontalLine*> CollectHorizontalLines(size_t nPage);

    CCustomState2 m_State;
};


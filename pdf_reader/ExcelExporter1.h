#pragma once

#include "CustomState2.h"

class CPdfDocument;

class CExcelExporter1
{
public:
    CExcelExporter1();
    ~CExcelExporter1();

    bool ProcessDocument(CPdfDocument* pDocument);


private:

    void ProcessAllTextNodes(const std::vector<TextAtLocation>& all_texts);
    std::vector<HorizontalLine*> CollectHorizontalLines(size_t nPage);

    CCustomState2 m_State;
};


#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfFont;

class CPdfPage :
    public CPdfObject
{
public:
    CPdfPage() = delete;
    CPdfPage(CPdfDocument* pDocument);

    virtual ~CPdfPage();

    pdf_type GetType() const final
    {
        return pdf_type::page;
    }
    
    bool operator==(const std::string&) final
    {
        return false;
    }

    const char* c_str() const final
    {
        return "";
    }

    std::vector<CPdfFont*> m_Fonts;
    std::vector<std::string> m_Contents;

    std::vector<int> m_ContentReference;

    CPdfDictionary* m_pDictionary;

    void Process();

    static CPdfPage* FromDictionary(CPdfDictionary* pDict);
};


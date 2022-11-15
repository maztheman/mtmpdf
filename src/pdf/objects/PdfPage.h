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

    virtual pdf_type GetType() const { return pdf_type::page; }
    virtual bool operator==(const std::string&) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    std::vector<CPdfFont*> m_Fonts;
    std::vector<std::string> m_Contents;

    std::vector<int> m_ContentReference;

    CPdfDictionary* m_pDictionary;

    void Process();

    static CPdfPage* FromDictionary(CPdfDictionary* pDict);
};


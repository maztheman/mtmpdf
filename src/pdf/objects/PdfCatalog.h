#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfPages;
class CPdfDocument;

class CPdfCatalog final :
    public CPdfObject
{
public:
    CPdfCatalog() = delete;
    CPdfCatalog(CPdfDocument* pDocument);
    virtual ~CPdfCatalog();

    pdf_type GetType() const override
    {
        return pdf_type::catalog;
    }

    bool operator==(const std::string&) override
    {
        return false;
    }

    const char* c_str() const override
    {
        return "";
    }

    void Process();
    
    static CPdfCatalog* FromDictionary(CPdfDictionary* pDict);

    CPdfPages* m_pPages = nullptr;
    CPdfDictionary* m_pDictionary = nullptr;

};


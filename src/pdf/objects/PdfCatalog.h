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

    virtual pdf_type GetType() const { return pdf_type::catalog; }
    virtual bool operator==(const std::string&) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    void Process();
    
    static CPdfCatalog* FromDictionary(CPdfDictionary* pDict);

    CPdfPages* m_pPages = nullptr;
    CPdfDictionary* m_pDictionary = nullptr;

};


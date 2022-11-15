#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfPage;

class CPdfPages :
    public CPdfObject
{
public:
    CPdfPages() = delete;
    CPdfPages(CPdfDocument* pDocument);

    virtual ~CPdfPages();
    
    virtual pdf_type GetType() const { return pdf_type::pages; }
    virtual bool operator==(const std::string&) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    void Process();

    static CPdfPages* FromDictionary(CPdfDictionary* pDict);

    CPdfPages* m_pParent = nullptr;
    std::vector<CPdfPage*> m_Data;
    CPdfDictionary* m_pDictionary;
};


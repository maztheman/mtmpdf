#pragma once
#include "PdfObject.h"

class CPdfDocument;
class CPdfDictionary;

class CPdfXRef :
    public CPdfObject
{
public:
    struct SObjectIndexLookup
    {
        uint32_t obj_stream;
        uint32_t obj_stream_index;
        uint32_t obj_index;
    };

    CPdfXRef(CPdfDocument* pDocument);
    virtual ~CPdfXRef();

    virtual pdf_type GetType() const { return pdf_type::xref; }
    virtual bool operator==(const std::string& value) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    std::vector<SObjectIndexLookup> m_ZipObjects;
    CPdfDictionary* m_pDictionary = nullptr;

    static CPdfXRef* FromDictionary(CPdfDictionary* pDict, char* data, char* end_data);
};


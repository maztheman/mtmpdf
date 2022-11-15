#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfDocument;

class CPdfObjectStream :
    public CPdfObject
{
public:

    struct SIndexLookup
    { 
        uint32_t obj;
        uint32_t offset;
    };


    CPdfObjectStream(CPdfDocument* pDocument);
    virtual ~CPdfObjectStream();

    virtual pdf_type GetType() const { return pdf_type::object_stream; }
    virtual bool operator==(const std::string& value) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    int N;
    int First;

    std::vector<SIndexLookup> m_Lookup;
    std::vector<SIndexLookup> m_SortedIndex;

    
    std::string Content;

    void ProcessObject(size_t index);


    static CPdfObjectStream* FromDictionary(CPdfDictionary* pDict, char* data);
};


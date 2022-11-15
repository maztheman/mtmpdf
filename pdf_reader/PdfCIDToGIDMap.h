#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfDocument;


class CPdfCIDToGIDMap :
    public CPdfObject
{
public:
    CPdfCIDToGIDMap(CPdfDocument* pDocument);
    virtual ~CPdfCIDToGIDMap();

    virtual pdf_type::pdf_type GetType() const { return pdf_type::cidtopid; }
    virtual bool operator==(const std::string& value) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    static CPdfCIDToGIDMap* FromDictionary(CPdfDictionary* pDict);
};


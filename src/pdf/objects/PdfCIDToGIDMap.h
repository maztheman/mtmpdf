#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfDocument;


class CPdfCIDToGIDMap final :
    public CPdfObject
{
public:
    CPdfCIDToGIDMap(CPdfDocument* pDocument);
    virtual ~CPdfCIDToGIDMap();

    pdf_type GetType() const final { return pdf_type::cidtopid; }
    bool operator==(const std::string& value) final { return false; }
    const char* c_str() const final {
        return "";
    };

    static CPdfCIDToGIDMap* FromDictionary(CPdfDictionary* pDict);
};


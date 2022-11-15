#pragma once
#include "PdfObject.h"

class CPdfString;

struct SCodeRange
{
    SCodeRange(int start, int end)
        : StartRange(start)
        , EndRange(end)
    {

    }

    int StartRange;
    int EndRange;
};

class CToUnicode :
    public CPdfObject
{
public:
    CToUnicode(CPdfDocument* pDocument);
    virtual ~CToUnicode();

    virtual pdf_type::pdf_type GetType() const { return pdf_type::tounicode; }
    virtual bool operator==(const std::string& value) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    std::string m_Data;

    void Process();

    std::vector<SCodeRange> Ranges;
    std::map<uint16_t, uint16_t> MappedGlyph;

    wchar_t GetMappedGlyph(wchar_t glyph);

    static CToUnicode* FromString(CPdfString* pDict);

};


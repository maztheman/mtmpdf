#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfDocument;


struct SFontWidthData
{
    std::map<char, int> CMap;
    std::vector<int> GlyphWidths;
    int UnitsPerEM;
};

class CTrueTypeFont :
    public CPdfObject
{
public:
    CTrueTypeFont() = delete;
    CTrueTypeFont(CPdfDocument* pDocument);
    virtual ~CTrueTypeFont();

    virtual pdf_type::pdf_type GetType() const { return pdf_type::true_type_font; }
    virtual bool operator==(const std::string& value) { return false; }
    virtual const char* c_str() const {
        return "";
    };

    std::string m_sName;
    std::string m_Data;

    SFontWidthData m_Widths;

    int GetGlyphWidth(char c);

    void Install();

    static CTrueTypeFont* FromMemory(CPdfDocument* pDocument, const std::string& data);
};


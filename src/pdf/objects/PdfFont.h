#pragma once

#include "PdfObject.h"

class CPdfDictionary;
class CPdfDocument;
class CTrueTypeFont;
class CPdfFontDescriptor;
class CToUnicode;



class CPdfFont 
    : public CPdfObject
{
public:
    CPdfFont() = delete;
    CPdfFont(CPdfDocument* pDocument);
    virtual ~CPdfFont();

    virtual pdf_type GetType() const { return pdf_type::font; }
    virtual bool operator==(const std::string& value) { (void)value; return false; }
    virtual const char* c_str() const {
        return "";
    };

    std::string m_sBaseFontName;
    std::string m_sName;
    std::string m_sAlias;

   // std::vector<ABCFLOAT> m_Widths;

    //Optional Pointers
    void Process();
    void SetSize(double size);

    double GetGlyphWidth(int c);

    CPdfDictionary* m_pDictionary;
    CTrueTypeFont* m_pTTFont = nullptr;
    CPdfFont* m_pDecendant = nullptr;
    CPdfFontDescriptor* m_pFontDescriptor = nullptr;
    CToUnicode* m_pToUnicode = nullptr;

    static CPdfFont* FromDictionary(CPdfDictionary* pDict);


private:
    void ProcessGlyphWidth();
};

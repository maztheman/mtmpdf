#pragma once
#include "PdfObject.h"

class CPdfDictionary;
class CPdfDocument;
class CPdfFont;

namespace font_type {
enum font_type
{
    internal,
    true_type
};

}


class CPdfFontDescriptor :
    public CPdfObject
{
public:
    CPdfFontDescriptor(CPdfDocument* pDocument);
    virtual ~CPdfFontDescriptor();

    virtual pdf_type::pdf_type GetType() const { return pdf_type::font_descriptor; }
    virtual bool operator==(const std::string& value) { return false; }
    virtual const char* c_str() const {
        return "";
    };


    font_type::font_type m_eFontType = font_type::internal;

    CPdfDictionary* m_pDictionary = nullptr;
    CPdfFont* m_pFont;

    void Process(CPdfFont* pFont);

    static CPdfFontDescriptor* FromDictionary(CPdfDictionary* pDict);


};


#include "PdfFont.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfFontDescriptor.h"
#include "PdfString.h"

#include "pdf/utils/ToUnicode.h"
#include "pdf/utils/TrueTypeFont.h"
#include "pdf/utils/tools.h"

using namespace std;

CPdfFont::CPdfFont(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfFont::~CPdfFont()
{
}

/*

<< 
    /BaseFont   /Helvetica-Bold
    /Encoding
    <<
        /BaseEncoding   /WinAnsiEncoding
        /Type           /Encoding
    >>
    /Name       /OPBaseFont4
    /Subtype    /Type1
    /Type       /Font
>>

*/
CPdfFont* CPdfFont::FromDictionary(CPdfDictionary* pDict)
{
    CPdfFont* pFont = new CPdfFont(pDict->m_pDocument);

    pFont->m_pDictionary = pDict;

    auto basefont = pDict->GetProperty("BaseFont");
    if (basefont) {
        pFont->m_sBaseFontName = basefont->c_str();
    }

    auto name = pDict->GetProperty("Name");
    if (name) {
        pFont->m_sName = name->c_str();
    }

    auto subtype = pDict->GetProperty("Subtype");
    if (subtype) {
        if (*subtype == "Type0") {
            auto tounicode = pDict->GetProperty("ToUnicode");
            if (tounicode) {
                int to_u = ObjectFromReference(tounicode);
                if (to_u != -1) {
                    pDict->m_pDocument->ProcessObject(to_u);
                    pFont->m_pToUnicode = CToUnicode::FromString(static_cast<CPdfString*>(pDict->m_pDocument->GetObjectData(to_u)));
                }

            }
        }
    }
    return pFont;
}

void CPdfFont::Process()
{
    auto subtype = m_pDictionary->GetProperty("Subtype");
    if (subtype) {
        if (*subtype == "Type0") {
            auto decendant_font = m_pDictionary->GetProperty("DescendantFonts");
            if (decendant_font) {
                auto font_space = split_on_space(decendant_font->c_str());
                for (auto it = font_space.begin(); it != font_space.end(); ++it) {
                    if (*it == "R") {
                        int obj = atoi((it - 2)->c_str());
                        m_pDocument->ProcessObject(obj);
                        CPdfFont* pDescendant = static_cast<CPdfFont*>(m_pDocument->GetObjectData(obj));
                        pDescendant->Process();
                        m_pDecendant = pDescendant;
                    }
                }


                int look = 0;

            }
        } else if (*subtype == "CIDFontType2") {
            auto cid2gid = m_pDictionary->GetProperty("CIDToGIDMap");
            if (cid2gid) {
                if (cid2gid->GetType() == pdf_type::dictionary) {
                    //easy

                } else if (cid2gid->GetType() == pdf_type::string) {
                    //reference
                    int cid_obj = ObjectFromReference(cid2gid);
                    if (cid_obj != -1) {
                        m_pDocument->ProcessObject(cid_obj);
                    }

                }


            }
        }
    }


    auto fd = m_pDictionary->GetProperty("FontDescriptor");
    if (fd) {
        int fd_obj = ObjectFromReference(fd);
        if (fd_obj != -1) {
            m_pDocument->ProcessObject(fd_obj);
            auto fd_ptr = static_cast<CPdfFontDescriptor*>(m_pDocument->GetObjectData(fd_obj));
            fd_ptr->Process(this);
            m_pFontDescriptor = fd_ptr;
        }
    }

    if (m_pToUnicode) {
        m_pToUnicode->Process();
    }


    ProcessGlyphWidth();
}

void CPdfFont::ProcessGlyphWidth()
{
    if (m_pFontDescriptor && m_pFontDescriptor->m_eFontType == font_type::true_type) {
        return;
    } 
    
    std::string font;
    bool bBold = false;
    if (m_sBaseFontName.find("Bold") != string::npos) {
        bBold = true;
        if (m_sBaseFontName.find("-Bold") != string::npos) {
            font = m_sBaseFontName.substr(0, m_sBaseFontName.find("-Bold"));
        } else {
            font = m_sBaseFontName.substr(0, m_sBaseFontName.find("Bold"));
        }
    } else {
        font = m_sBaseFontName;
    }

    if (font == "Times-Roman") {
        font = "Times New Roman";
    } else if (font == "Times") {
        font = "Times New Roman";
    }
    
    //HDC hdc = ::GetDC(NULL);
    //LOGFONTA lf = { 0 };

    //strcpy_s(lf.lfFaceName, font.c_str());
    //lf.lfHeight = -1000;
    //lf.lfWeight = bBold ? 700 : 400;

    //m_Widths.resize(256);

    //HFONT hFont = ::CreateFontIndirectA(&lf);
    //auto hOldFont = ::SelectObject(hdc, hFont);

    //GetCharABCWidthsFloatA(hdc, 0, 255, &m_Widths[0]);

    //::SelectObject(hdc, hOldFont);
    //::ReleaseDC(NULL, hdc);
}

void CPdfFont::SetSize(double size)
{
    /*


    if (m_pDecendant) {
        m_pDecendant->SetSize(size);
        return;
    }

    m_ptSize = size;

    HDC hdc = ::GetDC(NULL);
    LOGFONTA lf = { 0 };

    string font;
    bool bBold = false;

    if (m_pFontDescriptor && m_pFontDescriptor->m_eFontType == font_type::true_type) {
        font = m_pTTFont->m_sName;
    } else {
        if (m_sBaseFontName.find("Bold") != string::npos) {
            bBold = true;
            if (m_sBaseFontName.find("-Bold") != string::npos) {
                font = m_sBaseFontName.substr(0, m_sBaseFontName.find("-Bold"));
            } else {
                font = m_sBaseFontName.substr(0, m_sBaseFontName.find("Bold"));
            }
        } else {
            font = m_sBaseFontName;
        }

        if (font == "Times-Roman") {
            font = "Times New Roman";
        } else if (font == "Times") {
            font = "Times New Roman";
        }
    }

    strcpy_s(lf.lfFaceName, font.c_str());
    lf.lfHeight = -MulDiv((int)(size * 10), m_iLogPixelsY, 72) / 10;
    lf.lfWeight = bBold ? 700 : 400;

    HFONT hFont = ::CreateFontIndirectA(&lf);
    auto hOldFont = ::SelectObject(hdc, hFont);

    if (m_pFontDescriptor && m_pFontDescriptor->m_eFontType == font_type::true_type) {
        GetCharABCWidthsA(hdc, 0, 255, &m_ABCWidths[0]);
    } else {
        GetCharWidth32A(hdc, 0, 255, &m_Widths[0]);
    }

    ::SelectObject(hdc, hOldFont);
    ::ReleaseDC(NULL, hdc);
    */
}

double CPdfFont::GetGlyphWidth(int c)
{
    if (m_pDecendant) {
        return m_pDecendant->GetGlyphWidth(c);
    }
    if (m_pFontDescriptor && m_pFontDescriptor->m_eFontType == font_type::true_type) {
        auto w = m_pTTFont->GetGlyphWidth(c);
        return (w * 1000.0) / m_pTTFont->m_Widths.UnitsPerEM;
    } else {
        return 1.0;
        //return m_Widths[c].abcfA + m_Widths[c].abcfB + m_Widths[c].abcfC;
    }
}


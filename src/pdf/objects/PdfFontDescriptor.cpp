#include "PdfFontDescriptor.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfFont.h"
#include "PdfString.h"

#include "pdf/utils/TrueTypeFont.h"
#include "pdf/utils/tools.h"

CPdfFontDescriptor::CPdfFontDescriptor(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfFontDescriptor::~CPdfFontDescriptor()
{
}


CPdfFontDescriptor* CPdfFontDescriptor::FromDictionary(CPdfDictionary* pDict)
{
    auto doc = pDict->m_pDocument;
    auto obj = new CPdfFontDescriptor(doc);

    obj->m_pDictionary = pDict;
   

    return obj;
}

void CPdfFontDescriptor::Process(CPdfFont* pFont)
{
    m_pFont = pFont;//the parent font that is calling process

    auto fontfile2 = m_pDictionary->GetProperty("FontFile2");

    if (fontfile2) {
        //must be a TrueType Font
        //lets read it?
        int ff2_obj = ObjectFromReference(fontfile2);
        if (ff2_obj != -1) {
            m_pDocument->ProcessObject(ff2_obj);
            auto pTTF = static_cast<CPdfString*>(m_pDocument->GetObjectData(ff2_obj));
            auto ttf = CTrueTypeFont::FromMemory(m_pDocument, pTTF->m_Data);
            pFont->m_pTTFont = ttf;
            m_eFontType = font_type::true_type;
        }
    }

    auto cidset = m_pDictionary->GetProperty("CIDSet");
    if (cidset) {
        int cid_obj = ObjectFromReference(cidset);
        if (cid_obj != -1) {
            m_pDocument->ProcessObject(cid_obj);
        }
    }
}


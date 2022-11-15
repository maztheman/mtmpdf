#include "PdfPage.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfFont.h"

#include "pdf/utils/tools.h"

CPdfPage::CPdfPage(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfPage::~CPdfPage()
{
}

/*
<</Contents 88 0 R/CropBox[0 792 612 0]/MediaBox[0 792 612 0]/Resources<</ProcSet[/PDF/Text/ImageB/ImageC/ImageI]/Font<</F0 89 0 R/F1 90 0 R/F2 91 0 R/F3 92 0 R>>>>/Rotate 0/Type/Page/Parent 81 0 R>>


*/

CPdfPage* CPdfPage::FromDictionary(CPdfDictionary* pDict)
{
    auto doc = pDict->m_pDocument;
    auto retval = new CPdfPage(doc);
    retval->m_pDictionary = pDict;
    return retval;
}

void CPdfPage::Process()
{
    auto pResources = m_pDictionary->GetProperty("Resources");
    if (pResources) {
        CPdfDictionary* pFonts = nullptr;
        if (pResources->GetType() == pdf_type::dictionary) {
            auto pResourcesDict = static_cast<CPdfDictionary*>(pResources);
            auto ResFont = pResourcesDict->GetProperty("Font");
            if (ResFont) {
                if (ResFont->GetType() == pdf_type::string) {
                    int obj = ObjectFromReference(ResFont);
                    m_pDocument->ProcessObject(obj);
                    pFonts = static_cast<CPdfDictionary*>(m_pDocument->GetObjectData(obj));
                } else if (ResFont->GetType() == pdf_type::dictionary) {
                    pFonts = static_cast<CPdfDictionary*>(ResFont);
                } else {

                }
            }
            if (auto XObject = pResourcesDict->GetProperty("XObject"); XObject && XObject->GetType() == pdf_type::dictionary) {
                auto XObjectDict = static_cast<CPdfDictionary*>(XObject);
                for (auto& xo : *XObjectDict) {
                    int obj = ObjectFromReference(xo.second);
                    m_pDocument->ProcessObject(obj);
                }
                int look = 0;
            }
        } else if (pResources->GetType() == pdf_type::string) {
            int obj = ObjectFromReference(pResources);
            m_pDocument->ProcessObject(obj);
            auto obj_data = static_cast<CPdfDictionary*>(m_pDocument->GetObjectData(obj));
            pFonts = obj_data->GetDictionary("Font");
        } else {
            int res = 0;
        }
        if (pFonts) {
            for (auto& p : *pFonts) {
                int obj = ObjectFromReference(p.second);
                m_pDocument->ProcessObject(obj);
                auto font = static_cast<CPdfFont*>(m_pDocument->GetObjectData(obj));
                font->m_sAlias = p.first;
                font->Process();
                m_Fonts.push_back(font);
            }
        }
    }
    auto contents = m_pDictionary->GetProperty("Contents");
    if (contents) {
        //this goes thru a list of content pages.
        auto arr = split_on_space(contents->c_str());
        for (auto it = arr.begin(); it != arr.end(); ++it) {
            if (*it == "R") {
                int obj = atoi((it - 2)->c_str());
                m_pDocument->ProcessObject(obj);
                m_Contents.push_back(m_pDocument->GetObjectData(obj)->c_str());
                m_ContentReference.push_back(obj);
            }
        }
    }
}


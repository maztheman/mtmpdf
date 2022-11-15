#include "PdfCatalog.h"
#include "PdfDictionary.h"
#include "PdfPages.h"
#include "PdfDocument.h"

#include "pdf/utils/tools.h"

using namespace std;


CPdfCatalog::CPdfCatalog(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfCatalog::~CPdfCatalog()
{
}

void CPdfCatalog::Process()
{
    //Could be a reference OR it could be a dictionary
    auto val = m_pDictionary->GetValue();
    auto pages = val.find("Pages");
    CPdfPages* pPages = nullptr;
    switch (pages->second->GetType()) {
        case pdf_type::string:
        {
            int ref_obj = ObjectFromReference(pages->second);
            m_pDocument->ProcessObject(ref_obj);
            auto p_obj = m_pDocument->GetObjectData(ref_obj);
            if (p_obj) {
                pPages = static_cast<CPdfPages*>(p_obj);
            }
        }
        break;
        case pdf_type::dictionary:
        {
            pPages = CPdfPages::FromDictionary(m_pDictionary);
        }
        break;
    }

    m_pPages = pPages;

    pPages->Process();
}

CPdfCatalog* CPdfCatalog::FromDictionary(CPdfDictionary* pDict)
{
    auto doc = pDict->m_pDocument;
    CPdfCatalog* pCatalog = new CPdfCatalog(doc);
    pCatalog->m_pDictionary = pDict;
    return pCatalog;
}
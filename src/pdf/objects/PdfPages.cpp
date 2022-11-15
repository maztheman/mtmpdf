
#include "PdfPages.h"
#include "PdfDictionary.h"
#include "PdfDocument.h"
#include "PdfPage.h"

#include "pdf/utils/tools.h"

using namespace std;

/*
81 0 obj
<</Type / Pages / Count 5 / Kids[87 0 R 1 0 R 6 0 R 11 0 R 14 0 R] / Parent 84 0 R >>
endobj
82 0 obj
<</Type / Pages / Count 5 / Kids[17 0 R 20 0 R 23 0 R 25 0 R 28 0 R] / Parent 84 0 R >>
endobj
83 0 obj
<</Type / Pages / Count 3 / Kids[30 0 R 32 0 R 34 0 R] / Parent 84 0 R >>
endobj
84 0 obj
<</Type / Pages / Count 13 / Kids[81 0 R 82 0 R 83 0 R] >>
*/


CPdfPages::CPdfPages(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfPages::~CPdfPages()
{
}

void CPdfPages::Process()
{
    auto vals = m_pDictionary->GetValue();
    auto cnt = vals.find("Count");
    if (cnt == vals.end()) {
        return;
    }
    m_Data.resize(atoi(cnt->second->c_str()));
    auto kids = vals.find("Kids");
    if (kids == vals.end()) {
        return;
    }
    string sKids = trim_all(kids->second->c_str());
    auto data = split_on_space(sKids);
    int my_kid = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (*it == "R") {
            int ref_obj = atoi((it - 2)->c_str());
            m_pDocument->ProcessObject(ref_obj);
            auto a_kid = m_pDocument->GetObjectData(ref_obj);
            if (a_kid->GetType() == pdf_type::pages) {
                CPdfPages* pKid = static_cast<CPdfPages*>(a_kid);
                pKid->Process();
                pKid->m_pParent = this;
                for (auto& i : pKid->m_Data) {
                    m_Data[my_kid++] = i;
                }
            } else if (a_kid->GetType() == pdf_type::page) {
                m_Data[my_kid++] = static_cast<CPdfPage*>(a_kid);
            }
        }
    }

}

CPdfPages* CPdfPages::FromDictionary(CPdfDictionary* pDict)
{
    auto doc = pDict->m_pDocument;
    auto retval = new CPdfPages(doc);
    retval->m_pDictionary = pDict;
    retval->m_pParent = nullptr;

    return retval;
}


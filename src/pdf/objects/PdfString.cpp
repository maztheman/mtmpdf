#include "PdfString.h"

#include "pdf/utils/tools.h"

CPdfString::CPdfString(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfString::~CPdfString()
{
}

CPdfString* CPdfString::FromVector(CPdfDocument* pDocument, const std::vector<char>& data, size_t sz)
{
    CPdfString* pObj = new CPdfString(pDocument);
    pObj->m_Data = trim_all(std::string(data.begin(), data.end()));
    pObj->m_OriginalSize = sz;
    return pObj;
}

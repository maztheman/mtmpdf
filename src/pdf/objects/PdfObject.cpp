#include "PdfObject.h"
#include "PdfDocument.h"

#include "pdf/utils/tools.h"

CPdfObject::CPdfObject(CPdfDocument* pDocument)
    : m_pDocument(pDocument)
{
    m_pDocument->AddObject(this);
}

//add a reference to me to the document, please!  this way itll clean up the memory.

CPdfObject::~CPdfObject()
{
}


int ObjectFromReference(CPdfObject* pObj)
{
    if (pObj->GetType() != pdf_type::string) {
        return -1;
    }
    auto values = split_on_space(trim_all(pObj->c_str()));
    if (values.size() != 3 || values[2] != "R") {
        return -1;
    }
    return atoi(values[0].c_str());
}
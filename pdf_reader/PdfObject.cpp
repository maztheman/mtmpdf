#include "stdafx.h"
#include "PdfObject.h"
#include "PdfDocument.h"

CPdfObject::CPdfObject(CPdfDocument* pDocument)
    : m_pDocument(pDocument)
{
    m_pDocument->AddObject(this);
}

//add a reference to me to the document, please!  this way itll clean up the memory.

CPdfObject::~CPdfObject()
{
}

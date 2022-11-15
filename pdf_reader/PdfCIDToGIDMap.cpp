#include "stdafx.h"
#include "PdfCIDToGIDMap.h"


#include "PdfDocument.h"
#include "PdfDictionary.h"


CPdfCIDToGIDMap::CPdfCIDToGIDMap(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfCIDToGIDMap::~CPdfCIDToGIDMap()
{
}

CPdfCIDToGIDMap* CPdfCIDToGIDMap::FromDictionary(CPdfDictionary* pDict)
{
    auto doc = pDict->m_pDocument;
    auto obj = new CPdfCIDToGIDMap(doc);
    




    return obj;
}
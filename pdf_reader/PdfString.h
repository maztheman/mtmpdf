#pragma once
#include "PdfObject.h"
#include "tools.h"

class CPdfString : public CPdfObject
{
public:
    CPdfString(CPdfDocument* pDocument);
    virtual ~CPdfString();

    virtual pdf_type::pdf_type GetType() const final {
        return pdf_type::string;
    }

    virtual bool operator==(const std::string& value) final {
        return m_Data == value;
    }
    virtual const char* c_str() const final {
        return m_Data.c_str();
    }

    std::string m_Data;

    static CPdfString* FromVector(CPdfDocument* pDocument, const std::vector<char>& data) {
        CPdfString* pObj = new CPdfString(pDocument);
        pObj->m_Data = trim_all(std::string(data.begin(), data.end()));
        return pObj;
    }
};


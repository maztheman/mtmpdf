#pragma once
#include "PdfObject.h"

class CPdfDictionary : public CPdfObject
{
    std::map<std::string, CPdfObject*> m_Data;
public:
    CPdfDictionary() = delete;
    CPdfDictionary(CPdfDocument* pDocument);
    virtual ~CPdfDictionary();

    static CPdfDictionary* FromString(CPdfDocument* pDocument, std::string sValue);
    
    static CPdfDictionary* FromRawString(CPdfDocument* pDocument, char*& first, char* last);

    const std::map<std::string, CPdfObject*>& GetValue() const {
        return m_Data;
    }

    CPdfObject* GetProperty(const std::string& value) {
        auto it = m_Data.find(value);
        return it != m_Data.end() ? it->second : nullptr;
    }

    CPdfDictionary* GetDictionary(const std::string& value) {
        auto obj = GetProperty(value);
        if (obj != nullptr) {
            if (obj->GetType() == pdf_type::dictionary) {
                return static_cast<CPdfDictionary*>(obj);
            }
        }
        return nullptr;
    }

    auto begin() {
        return m_Data.begin();
    }

    auto end() {
        return m_Data.end();
    }

    virtual pdf_type::pdf_type GetType() const final {
        return pdf_type::dictionary;
    }

    virtual bool operator==(const std::string& value) final {
        return false;
    }

    virtual const char* c_str() const final {
        return "";
    }
    
};


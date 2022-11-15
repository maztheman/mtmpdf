#pragma once


enum class pdf_type
{
    string,
    dictionary,
    catalog,
    pages,
    font,
    page,
    object_stream,
    xref,
    cidtopid,
    font_descriptor,
    true_type_font,
    tounicode
};

class CPdfDocument;

class CPdfObject
{
public:
    CPdfObject() = delete;
    CPdfObject(CPdfDocument* pDocument);
    virtual ~CPdfObject();

    virtual pdf_type GetType() const = 0;
    virtual bool operator==(const std::string& value) = 0;
    virtual const char* c_str() const = 0;

    bool operator==(const char* value) {
        return *this == std::string(value);
    }


    CPdfDocument* m_pDocument;

};

int ObjectFromReference(CPdfObject* pObj);

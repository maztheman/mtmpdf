#include "PdfObjectStream.h"

#include <algorithm>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfString.h"

#include "pdf/utils/tools.h"
#include "pdf/utils/zlib_helper.h"

using namespace std;

CPdfObjectStream::CPdfObjectStream(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
    N = 0;
}


CPdfObjectStream::~CPdfObjectStream()
{
}

CPdfObjectStream* CPdfObjectStream::FromDictionary(CPdfDictionary* pDict, char* data)
{
    auto obj = new CPdfObjectStream(pDict->m_pDocument);

    auto pN = pDict->GetProperty("N");
    if (pN) {
        obj->N = atoi(pN->c_str());
    }
    auto pFirst = pDict->GetProperty("First");
    if (pFirst) {
        obj->First = atoi(pFirst->c_str());
    }

    std::string content;
    auto pFilter = pDict->GetProperty("Filter");
    if (pFilter) {
        if (*pFilter == "FlateDecode") {
            auto length = pDict->GetProperty("Length");
            auto len = atoi(length->c_str());
            
            if (auto current = preview_next_line(data); current.find("stream") != string::npos) {
                get_next_line(&data);
                data = skip_space(data);
            } else {
                data = skip_space(data);
            }
            
            vector<char> buffer(len);
            memcpy(&buffer[0], data, len);
            vector<char> dest;
            auto rc = InflateData(buffer, dest);
            if (rc == -3) {
                return obj;
            }
            content = string(dest.begin(), dest.end());
        }
    } else {

    }

    auto& lookup = obj->m_Lookup;
    auto& sorted = obj->m_SortedIndex;
    lookup.reserve(obj->N);
    obj->Content = std::move(content);
    std::string hmm(obj->Content.begin(), obj->Content.begin() + obj->First);

    auto ii = split_on_space(hmm);
    for (size_t n = 0; n < ii.size(); n += 2) {
        uint32_t i_obj = atoi(ii[n].c_str());
        uint32_t c_obj = atoi(ii[n + 1].c_str());
        lookup.push_back(SIndexLookup{i_obj, c_obj});
    }

    sorted = lookup;

    std::sort(sorted.begin(), sorted.end(), [&](const auto& left, const auto& right) {
        return left.offset < right.offset;
    });

    return obj;
}

void CPdfObjectStream::ProcessObject(size_t index)
{
    auto& obj_off = m_Lookup[index];

    auto hmm = std::find_if(m_SortedIndex.begin(), m_SortedIndex.end(), [&obj_off](const auto& a) {
        return a.obj == obj_off.obj;
    });

    ++hmm;



    char* data = &Content[obj_off.offset + First];
    char* end_data = nullptr;

    if (hmm != m_SortedIndex.end()) {
        end_data = &Content[hmm->offset + First];
    } else {
        end_data = &Content[Content.size() - 1];
    }

    m_pDocument->ProcessInternalObject(obj_off.obj, data, end_data);
}

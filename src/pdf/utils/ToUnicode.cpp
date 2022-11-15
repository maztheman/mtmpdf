#include "ToUnicode.h"

#include "pdf/objects/PdfDocument.h"
#include "pdf/objects/PdfString.h"
#include "pdf/utils/tools.h"

CToUnicode::CToUnicode(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CToUnicode::~CToUnicode()
{
}

CToUnicode* CToUnicode::FromString(CPdfString* pDict)
{
    auto obj = new CToUnicode(pDict->m_pDocument);
    obj->m_Data = pDict->m_Data;
    return obj;
}

void CToUnicode::Process()
{
    auto lines = split_on_space(m_Data);
    
    for (size_t n = 0; n < lines.size(); n++) {
        auto& line = lines[n];
        if (line == "begincodespacerange") {
            int scr = atoi(lines[n - 1].c_str());
            for (int i = 0; i < scr; i++) {
                auto& st_range = lines[n + 1];
                auto& ed_range = lines[n + 2];

                int nStRange = 0;
                int nEdRange = 0;

                if (st_range[0] == '<') {
                    auto arr = HexTo16BitArray(st_range.substr(1, st_range.find('>') - 1));
                    nStRange = arr[0];
                }

                if (ed_range[0] == '<') {
                    auto arr = HexTo16BitArray(ed_range.substr(1, ed_range.find('>') - 1));
                    nEdRange = arr[0];
                }

                Ranges.emplace_back(nStRange, nEdRange);
            }
        } else if (line == "beginbfchar") {
            auto& st_range = lines[n + 1];
            auto& ed_range = lines[n + 2];

            uint16_t nStRange = 0;
            uint16_t nEdRange = 0;

            if (st_range[0] == '<') {
                auto arr = HexTo16BitArray(st_range.substr(1, st_range.find('>') - 1));
                nStRange = arr[0];
            }

            if (ed_range[0] == '<') {
                auto arr = HexTo16BitArray(ed_range.substr(1, ed_range.find('>') - 1));
                nEdRange = arr[0];
            }

            MappedGlyph.emplace(nStRange, nEdRange);
        }
    }
    
    int y = 0;
}

wchar_t CToUnicode::GetMappedGlyph(wchar_t glyph)
{
    auto it = MappedGlyph.find(glyph);
    return it != MappedGlyph.end() ? it->second : glyph;

}


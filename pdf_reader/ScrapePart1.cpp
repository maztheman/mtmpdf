#include "stdafx.h"
#include "ScrapePart1.h"

#include "PdfDocument.h"

#include "tools.h"
#include "CSVExporter.h"

using namespace std;

CScrapePart1::CScrapePart1()
{
}


CScrapePart1::~CScrapePart1()
{
}


const TextAtLocation* FindStartPart1(const vector<TextAtLocation>& all_texts, size_t& nIndex)
{
    nIndex = 0;
    for (auto& text : all_texts) {
        if (trim_all(text.text) == "PART I") {
            return &text;
        }
        nIndex++;
    }
    nIndex = ~0;
    return nullptr;
}

const TextAtLocation* FindEndPart1(const vector<TextAtLocation>& all_texts, size_t& nIndex)
{
    nIndex = 0;
    for (auto& text : all_texts) {
        if (trim_all(text.text) == "END OF PART I") {
            return &text;
        }
        nIndex++;
    }
    nIndex = ~0;
    return nullptr;
}



void CScrapePart1::ProcessDocument(CPdfDocument* pDocument)
{
    char drive[8];
    char dir[280];
    char name[280];
    char ext[32];

    _splitpath_s(pDocument->GetFilename().c_str(), drive, dir, name, ext);


    pDocument->CollectAllTextNodes([&](const auto& left, const auto& right) {
        size_t nLeftPage = left.nPage;
        size_t nRightPage = right.nPage;

        if (nLeftPage < nRightPage) {
            return true;
        } else if (nLeftPage > nRightPage) {
            return false;
        }

        Matrix trm_left = left.GS.GetUserSpaceMatrix();
        Matrix trm_right = right.GS.GetUserSpaceMatrix();

        double dLeftY = trm_left.y;
        double dRightY = trm_right.y;

        double dLeftX = trm_left.x;
        double dRightX = trm_right.x;


        if (dLeftY > dRightY) {
            return false;
        } else if (dLeftY < dRightY) {
            return true;
        }

        return dLeftX < dRightX;
    });
    auto& all_texts = pDocument->GetAllTextNodes();

    size_t first_index;
    size_t last_index;

    auto first = FindStartPart1(all_texts, first_index);
    auto last = FindEndPart1(all_texts, last_index);

    vector<const TextAtLocation*> lines;

    vector<TextAtLocation*> del_temp;

    auto start_at_top = first->GS.GetUserSpaceMatrix();

    start_at_top.y -= abs(start_at_top.d * first->GS.TextState.Tfs);

    for (auto n = first_index; n != last_index; n++) {
        auto& text = all_texts[n];
        
        auto hj = &text;

        auto line_size = text.GS.TextState.Tfs * abs(text.GS.TextState.Tm.d);
        auto top = text.GS.GetUserSpaceMatrix();
        
        const TextAtLocation* last_item = nullptr;
        
        if (lines.empty() == false) {
            last_item = lines.back();
        }

        //merge down
        if (last_item) {
            auto test_back = last_item->GS.GetUserSpaceMatrix();
            if ((top.y - line_size) <= test_back.y && top.y >= test_back.y) {
                lines.erase(lines.begin() + lines.size() - 1);
                auto ggg = new TextAtLocation(text);
                del_temp.push_back(ggg);
                if (top.x < test_back.x) {
                    ggg->text += last_item->text;
                } else {
                    ggg->text = last_item->text + ggg->text;
                }
                hj = ggg;
            }
        }

        if (start_at_top.y <= top.y) {
            lines.push_back(hj);
        }
    }

    vector<string> all_lines;
    auto dLastY = lines[0]->GS.TextState.Tm.y;
    string cur;
    for (auto& line : lines) {
        auto dCurrentY = line->GS.TextState.Tm.y;
        if (dLastY == dCurrentY) {
        } else {
            if (cur.empty() == false) {
                all_lines.emplace_back(trim_all(cur));
                cur = "";
            }
        }
        cur += line->text;
        dLastY = dCurrentY;
    }

    vector<string> paragraphs;
    string paragraph;
    for (auto& line : all_lines) {
        if (line.empty()) {
            auto hh = trim_all(paragraph);
            if (hh.empty() == false) {
                paragraphs.emplace_back(std::move(hh));
            }
            paragraph = "";
        }
        if (line.size() < 3) {
            paragraph += line + " ";
        } else {
            paragraph += line + "\n";
        }
    }

    if (paragraph.empty() == false) {
        paragraphs.emplace_back(std::move(paragraph));
    }


    string sOutputFileName;
    sOutputFileName += drive;
    sOutputFileName += dir;
    sOutputFileName += name;
    sOutputFileName += ".csv";

    CCSVExporter::ToFile(sOutputFileName, paragraphs);
}

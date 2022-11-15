#include "stdafx.h"
#include "ExcelExporter1.h"

#include "ExcelExporter.h"

#include "PdfDocument.h"

using namespace std;

static bool TryFindAllDescription(const vector<TextAtLocation>& texts, vector<size_t>& descrs)
{
    size_t index = 0;
    for (auto& text : texts) {
        if (_strnicmp(text.text.c_str(), "description", 11) == 0) {
            descrs.push_back(index);
        }
        index++;
    }
    return descrs.empty() == false;
}

static std::vector<const TextAtLocation*> GetAllHeaderItems(const vector<TextAtLocation>& texts, const TextAtLocation* pSampleHeaderItem)
{
    std::vector<const TextAtLocation*> header_items;

    auto top = pSampleHeaderItem->GS.GetUserSpaceMatrix();

    for (auto& text : texts) {

        auto test_top = text.GS.GetUserSpaceMatrix();

        auto diff = abs(top.y - test_top.y);

        if (diff < 1.0 && text.nPage == pSampleHeaderItem->nPage) {
            header_items.push_back(&text);
        }
    }

    sort(header_items.begin(), header_items.end(), [&](const TextAtLocation*& a, const TextAtLocation*& b) {
        return a->GS.TextState.Tm.x < b->GS.TextState.Tm.x;
    });

    return header_items;
}


CExcelExporter1::CExcelExporter1()
{
}


CExcelExporter1::~CExcelExporter1()
{
}

bool CExcelExporter1::ProcessDocument(CPdfDocument* pDocument)
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

        double diff_y = abs(dLeftY - dRightY);

        bool is_same = diff_y < 4.0;

        double dLeftX = trm_left.x;
        double dRightX = trm_right.x;

        if (is_same) {
            return dLeftX < dRightX;
        }

        if (dLeftY > dRightY) {
            return false;
        } else if (dLeftY < dRightY) {
            return true;
        }
    });
    auto& all_texts = pDocument->GetAllTextNodes();
    m_State.vertical_lines = pDocument->GetAllVerticalLines();
    m_State.horizontal_lines = pDocument->GetAllHorizontalLines();
    
    ProcessAllTextNodes(all_texts);

    string sOutputFileName;
    sOutputFileName += drive;
    sOutputFileName += dir;
    sOutputFileName += name;
    sOutputFileName += ".xls";


    return CExcelExporter::ToFile(sOutputFileName, m_State);
}

static inline const HorizontalLine* FindUnderlined(vector<HorizontalLine*>& lines, double x, double y)
{
    //Find the closest to the left of text
    double dx = 9999.0;
    
    deque<size_t> sizes;

    size_t n = 0;
    for (auto& line : lines) {
        auto diff = abs(line->y - y);
        if (diff < 8.0) {
            if (line->x < x) {
                auto g = abs(line->x - x);
                if (g < dx) {
                    dx = g;
                    sizes.push_back(n);
                }
            }
        }
        n++;
    }

    if (sizes.empty() == false) {
        return lines[sizes.back()];
    }

    return nullptr;
}

static inline size_t GetColumnIndex(const std::vector<PdfColumn>& cols, double x)
{
    size_t n = 0;
    for (auto& col : cols) {
        if (col.has_maybe_rect) {
            if (col.maybe_rect.x < x && (col.maybe_rect.x + col.maybe_rect.width) > x) {
                return n;
            }
        }
        n++;
    }
    return ~0;
}

void CExcelExporter1::ProcessAllTextNodes(const vector<TextAtLocation>& all_texts)
{
    //all the pages are here, so we can do multi page-trace backs and look forwards
    //find all begin header stuff
    vector<size_t> arDescrIndexs;
    if (TryFindAllDescription(all_texts, arDescrIndexs)) {
        for (size_t n = 0; n < arDescrIndexs.size(); n++) {
            auto first = arDescrIndexs[n];
            auto& descr = all_texts[first];
            auto last = all_texts.size();
            if ((n + 1) < arDescrIndexs.size()) {
                last = arDescrIndexs[n + 1];
            }

            auto header_items = GetAllHeaderItems(all_texts, &descr);
            auto horz = CollectHorizontalLines(descr.nPage);

            double header_bottom_floor = -9999;

            std::vector<PdfColumn>& cols = m_State.columns;
            cols.resize(header_items.size());

            size_t uCol = 0;
            for (auto& header : header_items) {
                auto header_b = header->GS.GetUserSpaceMatrix();

                auto the_underlined = FindUnderlined(horz, header->GS.TextState.Tm.x, header->GS.TextState.Tm.y);
                auto rect = Measure(header->GS, header->text);
                
                header_bottom_floor = max(header_bottom_floor, header_b.y);

                cols[uCol].name = header->text;
                if (the_underlined) {
                    double px_width = the_underlined->x * the_underlined->Cmd;

                    cols[uCol].has_maybe_rect = true;
                    cols[uCol].maybe_rect = { the_underlined->x, the_underlined->y,  the_underlined->width, 1.0 };
                    double dMidText = rect.x + (rect.width / 2.0);
                    double dMidMaybe = cols[uCol].maybe_rect.x + (cols[uCol].maybe_rect.width / 2.0);
                    cols[uCol].header_is_centered = abs(dMidText - dMidMaybe) < 8.0;
                } else {
                    cols[uCol].has_maybe_rect = false;
                    cols[uCol].header_is_centered = false;
                }

                uCol++;
            }

            m_State.tables.resize(1);
            //Ok so we found the headers, then converted to a column having the width of the underline as a col guide.

            m_State.tables[0].columns = cols;
            m_State.Row.resize(cols.size());
            size_t nLastCol = ~0;
            bool StartLogging = false;
            for (size_t row = first; row != last; ++row) {
                auto& text_row = all_texts[row];

                auto text_bottom = text_row.GS.GetUserSpaceMatrix();

                if (text_row.text == "55207587128") {
                    int yyyy = 0;
                    StartLogging = true;
                }

                if (StartLogging) {
                    printf("US(x,y) = [%g,%g] PDF(x,y) = [%g,%g]\n", text_bottom.x, text_bottom.y, text_row.GS.TextState.Tm.x, text_row.GS.TextState.Tm.y);
                }

                if (text_bottom.y <= header_bottom_floor) {
                    continue;
                }

                auto TheCol = GetColumnIndex(cols, text_row.GS.TextState.Tm.x);
                if (TheCol != ~0) {
                    if (nLastCol == ~0) {
                        //last col was null so ignore
                    } else {
                        if (TheCol < nLastCol) {
                            if (m_State.Row.empty() == false) {
                                int cnt = 0;
                                int sensible_ratio = m_State.Row.size() * 0.3;
                                for (auto& gg : m_State.Row) {
                                    if (gg.empty() == false) {
                                        cnt++;
                                    }
                                }
                                if (cnt > sensible_ratio) {
                                    m_State.tables[0].rows.emplace_back(std::move(m_State.Row));
                                }
                            }
                        }
                    }
                    m_State.Row.resize(cols.size());
                    if (m_State.Row[TheCol].empty() == false && text_row.text.empty() == false) {
                        m_State.Row[TheCol] += "\n";
                    }
                    m_State.Row[TheCol] += text_row.text;
                    if (TheCol == (cols.size() - 1)) {
                        if (m_State.Row.empty() == false) {
                            m_State.tables[0].rows.emplace_back(std::move(m_State.Row));
                        }
                    }
                }

                nLastCol = TheCol;
            }
        }
    }
}

vector<HorizontalLine*> CExcelExporter1::CollectHorizontalLines(size_t nPage)
{
    vector<HorizontalLine*> retval;
    for (auto&h : m_State.horizontal_lines) {
        if (h.nPage == nPage) {
            retval.push_back(&h);
        }
    }

    sort(retval.begin(), retval.end(), [&](const auto& left, const auto& right)
    {
        return left->y > right->y;
    });

    return retval;
}


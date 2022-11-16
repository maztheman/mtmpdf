#include "PdfToExcel.h"

#include "pdf/objects/PdfDocument.h"
#include "pdf/objects/PdfFont.h"
#include "pdf/utils/tools.h"

#include "ExcelExporter.h"

#include <processor/pdf/Processor.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace std;



static inline int match_columns(const vector<PdfColumn>& test, vector<PdfTable>& tables, const std::string& trade)
{
    bool bFound = false;
    int iTbl = 0;
    for (auto& tbl : tables) {
        if (tbl.trade == trade && test.size() == tbl.columns.size()) {
            auto size = tbl.columns.size();
            bool bDiff = false;
            for (size_t i = 0; i < size && bDiff == false; i++) {
                if (tbl.columns[i].name != test[i].name) {
                    bDiff = true;
                }
            }
            if (bDiff == false) {
                bFound = true;
                break;
            }
        }
        iTbl++;
    }
    if (bFound == false) {
        PdfTable tbl;
        tbl.columns = test;
        tbl.trade = trade;
        tables.emplace_back(std::move(tbl));
        iTbl = static_cast<int>(tables.size() - 1);
    }
    return iTbl;
}

CPdfToExcel::CPdfToExcel()
{
}


CPdfToExcel::~CPdfToExcel()
{
}

static inline bool is_all_upper_case(const std::string& val)
{
    for (auto c : val) {
        if (isspace(c)) {
        } else if (c >= 'a' && c <= 'z') {
            return false;
        }
    }
    return true;
}

static inline bool is_no_data_last_row(const std::vector<std::string>& Row) {
    auto size = Row.size();
    for (size_t i = 1; i < size; i++) {
        if (Row[i].empty() == false) {
            return false;
        }
    }
    return true;
}

bool CPdfToExcel::ProcessRows(const processor::pdf::types::TextAtLocation& text_data, size_t& iLastCol)
{
    double dRowY = text_data.GS.TextState.Tm.y * text_data.GS.TextState.Tm.d;
    if (m_State.dHeaderY <= dRowY) {
        //its a header item, ignore
        return true;
    }

    bool rc = false;

    const auto& text = text_data.text;

    auto rect = Measure(text_data.GS, text);
    size_t iTbl = static_cast<size_t>(match_columns(m_State.columns, m_State.tables, m_State.locked_trade));
    auto& tbl = m_State.tables[iTbl];
    //check to see if its text or a number
    if (is_number(text)) {
        //right aligned
        size_t iCol = 0;
        for (auto& col : m_State.columns) {
            bool bMatched = false;
            if (col.has_maybe_rect) {
                bMatched = (rect.x > col.maybe_rect.x) && (rect.x < (col.maybe_rect.x + col.maybe_rect.width));
            } else {
                auto fuzzy_left = iCol == 0 ? (col.rect.x - 5.0) : (m_State.columns[iCol - 1].rect.x + m_State.columns[iCol - 1].rect.width);// ;
                auto fuzzy_right = col.rect.x + col.rect.width + 5.0;
                bMatched = rect.x >= fuzzy_left && rect.x <= fuzzy_right &&
                    (rect.x + rect.width) >= fuzzy_left && (rect.x + rect.width) <= fuzzy_right;
            }
            //lets be a litte more skeptical about the columns
            if (bMatched) {
                rc = true;
                if (iCol < iLastCol) {
                    if (m_State.Row.empty() == false) {
                        tbl.rows.emplace_back(std::move(m_State.Row));
                        m_State.iRow++;
                    }
                }
                m_State.Row.resize(m_State.columns.size());
                if (m_State.Row[iCol].empty() == false) {
                    m_State.Row[iCol] += " ";
                }
                m_State.Row[iCol] += text;
                if (iCol == (m_State.columns.size() - 1)) {
                    tbl.rows.emplace_back(std::move(m_State.Row));
                    m_State.iRow++;
                    iLastCol = ~0UL;
                } else {
                    iLastCol = iCol;
                }
                break;
            }
            iCol++;
        }
    } else {
        size_t iCol = 0;
        for (auto& col : m_State.columns) {
            bool bMatched = false;
            if (col.has_maybe_rect) {
                bMatched = (rect.x > col.maybe_rect.x) && (rect.x < (col.maybe_rect.x + col.maybe_rect.width));
            } else {
                bMatched = col.rect.x == rect.x;
            }
            if (bMatched) {
                rc = true;
                if (iCol < iLastCol) {
                    //possible end of row
                    if (m_State.Row.empty() == false) {
                        tbl.rows.emplace_back(std::move(m_State.Row));
                        m_State.iRow++;
                    }
                }

                if (text.size() > 7 && text.substr(0, 7) == "Totals:") {
                    tbl.trade = trim_all(text.substr(7));
                    m_State.locked_trade = tbl.trade;
                    if (m_State.Row.empty() == false) {
                        if (is_no_data_last_row(m_State.Row) && m_State.Row[0].empty() == false) {
                            if (tbl.rows.empty() == false) {
                                auto& lr = tbl.rows.back();
                                lr[iLastCol] += "\n" + m_State.Row[iCol];
                                m_State.Row[iCol] = "";
                            } else {
                                m_State.Row[iCol] += " ";;
                            }
                        }
                    }
                }

                if (text.size() > 5 && text.substr(0, 5) == "Page:") {
                    m_State.Row.clear();
                    return rc;
                }

                if (text.substr(0, 6) == "Total:") {
                }

                m_State.Row.resize(m_State.columns.size());
                if (m_State.Row[iCol].empty() == false) {
                    if (m_State.iRow == 0 && iCol == 0 && is_all_upper_case(m_State.Row[iCol])) {
                        tbl.coverage = m_State.Row[iCol];
                        m_State.Row[iCol] = "";
                    } else if (iLastCol == 0 && is_no_data_last_row(m_State.Row)) {
                        //tbl.rows.emplace_back(std::move(m_State.Row));
                        //m_State.Row.resize(m_State.columns.size());
                        if (tbl.rows.empty() == false) {
                            auto& lr = tbl.rows.back();
                            lr[iLastCol] += "\n" + m_State.Row[iCol];
                            m_State.Row[iCol] = "";
                        } else {
                            m_State.Row[iCol] += " ";;
                        }
                    } else {
                        m_State.Row[iCol] += "\n";
                    }
                }
                m_State.Row[iCol] += text;
                if (iCol == (m_State.columns.size() - 1)) {
                    if (m_State.Row.empty() == false) {
                        tbl.rows.emplace_back(std::move(m_State.Row));
                        m_State.iRow++;
                    }
                    iLastCol = ~0UL;
                } else {
                    iLastCol = iCol;
                }
                break;
            } else if (iCol > 0) {
                auto fuzzy_left = (m_State.columns[iCol - 1].rect.x + m_State.columns[iCol - 1].rect.width) - 8.0;// ;
                auto fuzzy_right = col.rect.x + col.rect.width + 8.0;

                if (rect.x >= fuzzy_left && rect.x <= fuzzy_right &&
                    (rect.x + rect.width) >= fuzzy_left && (rect.x + rect.width) <= fuzzy_right) {
                    rc = true;
                    if (iCol < iLastCol) {
                        if (m_State.Row.empty() == false) {
                            tbl.rows.emplace_back(std::move(m_State.Row));
                            m_State.iRow++;
                        }
                    }

                    if (text.size() > 5 && text.substr(0, 5) == "Page:") {
                        m_State.Row.clear();
                        return true;
                    }
                    m_State.Row.resize(m_State.columns.size());
                    if (m_State.Row[iCol].empty() == false) {
                        m_State.Row[iCol] += " ";
                    }
                    m_State.Row[iCol] += text;
                    if (iCol == (m_State.columns.size() - 1)) {
                        if (m_State.Row.empty() == false) {
                            tbl.rows.emplace_back(std::move(m_State.Row));
                            m_State.iRow++;
                        }
                        iLastCol = ~0UL;
                    } else {
                        iLastCol = iCol;
                    }
                    break;
                }
            }
            iCol++;
        }
    }

    return rc;
}

static bool TryFindAllDescription(const vector<processor::pdf::types::TextAtLocation>& texts, vector<size_t>& descrs)
{
    size_t index = 0;
    for (auto& text : texts) {
        if (iequals(text.text.c_str(), "description") == 0) {
            descrs.push_back(index);
        }
        index++;
    }
    return descrs.empty() == false;
}

static std::vector<const processor::pdf::types::TextAtLocation*> GetAllHeaderItems(const vector<processor::pdf::types::TextAtLocation>& texts, const processor::pdf::types::TextAtLocation* pSampleHeaderItem)
{
    std::vector<const processor::pdf::types::TextAtLocation*> header_items;

    auto top = pSampleHeaderItem->GS.GetUserSpaceMatrix();

    for (auto& text : texts) {

        auto test_top = text.GS.GetUserSpaceMatrix();

        auto diff = abs(top.y - test_top.y);

        if (diff < 1.0 && text.nPage == pSampleHeaderItem->nPage) {
            header_items.push_back(&text);
        }
    }

    sort(header_items.begin(), header_items.end(), [&](const processor::pdf::types::TextAtLocation*& a, const processor::pdf::types::TextAtLocation*& b) {
        return a->GS.TextState.Tm.x < b->GS.TextState.Tm.x;
    });

    return header_items;
}


static const processor::pdf::types::TextAtLocation* FindPageText(const vector<processor::pdf::types::TextAtLocation>& texts, const processor::pdf::types::TextAtLocation* pCurrentLocation)
{
    for (auto& text : texts) {
        if (text.nPage == pCurrentLocation->nPage) {
            if (text.text.size() > 5 && text.text.substr(0, 5) == "Page:") {
                return &text;
                break;
            }
        }
    }
    return nullptr;
}


static const processor::pdf::types::TextAtLocation* FindLineItemTotals(const vector<processor::pdf::types::TextAtLocation>& texts)
{
    for (auto& text : texts) {
        if (text.text.substr(0, 17) == "Line Item Totals:") {
            return &text;
        }
    }
    return nullptr;
}

void CPdfToExcel::SetupColumns(std::vector<const processor::pdf::types::TextAtLocation*>& header_items)
{
    m_State.columns.clear();
    m_State.iRow = 0;

    m_State.locked_trade = m_State.trade;
    if (m_State.locked_trade.find("CONTINUED -") != string::npos) {
        m_State.locked_trade = m_State.locked_trade.substr(12);
    }

    for (auto& header : header_items) {
        const auto& GS = header->GS;
        auto rect = Measure(GS, header->text);

        auto test1 = m_State.FindClosestToTopLeft(GS.TextState.Tm.x, GS.TextState.Tm.y, header->nPage);
        auto test2 = m_State.FindClosestToTopRight(GS.TextState.Tm.x, GS.TextState.Tm.y, header->nPage);

        PdfRect maybe_rect{ 0.0, 0.0, 0.0, 0.0 };
        bool has_maybe_rect = false;
        bool is_centered = false;
        if (test1 && test2) {
            maybe_rect.x = test1->x;
            maybe_rect.y = test1->y;
            maybe_rect.width = test2->x - test1->x;
            maybe_rect.height = 0;
            has_maybe_rect = true;
            double dMidText = rect.x + (rect.width / 2.0);
            double dMidMaybe = maybe_rect.x + (maybe_rect.width / 2.0);
            is_centered = abs(dMidText - dMidMaybe) < 8.0;
        }

        m_State.dHeaderY = rect.y * GS.TextState.Tm.d;
        m_State.columns.emplace_back(PdfColumn{ rect, maybe_rect, has_maybe_rect, is_centered, header->text });
    }

    m_State.Row.clear();
    m_State.Row.resize(m_State.columns.size());
}

void CPdfToExcel::TraceBackForTrade(const vector<processor::pdf::types::TextAtLocation>& texts, size_t header_index)
{
    const auto& descr = texts[header_index];
    auto header_y = descr.GS.TextState.Tm.y;
    for (size_t n = header_index - 1; n != ~0UL; n--) {
        const auto& o = texts[n];
        auto ya = o.GS.TextState.Tm.y;
        if (ya > header_y) {
            if (o.GS.TextState.Tfont->m_sBaseFontName.find("Bold") != string::npos) {
                if (trim_all(o.text).size() > 2) {
                    m_State.trade = o.text;
                    break;
                }
            }
        }
    }
}

bool CPdfToExcel::ProcessDocument(CPdfDocument* pDocument)
{
    fs::path outPath = pDocument->GetFilename();

    auto processedText = processor::pdf::ProcessText(pDocument);

    std::sort(processedText.AllTexts.begin(), processedText.AllTexts.end(), [&](const auto& left, const auto& right) {
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

    
    auto& all_texts = processedText.AllTexts;
    m_State.vertical_lines = processedText.AllVerticalLines;
    m_State.horizontal_lines = processedText.AllHorizontalLines;
    m_State.media_boxes = pDocument->GetAllMediaBoxes();
    ProcessAllTextNodes(all_texts);

    outPath.replace_extension(".xls");

    return CExcelExporter::ToFile(outPath.generic_string(), m_State);
}

const processor::pdf::types::TextAtLocation* FindNextTradeItem(const vector<processor::pdf::types::TextAtLocation>& all_texts, size_t first, size_t last)
{

    for (; first != last; ++first) {
        if (all_texts[first].text.size() > 7 && all_texts[first].text.substr(0, 7) == "Totals:") {
            return &all_texts[first];
        }
    }

    return nullptr;
}

vector<processor::types::HorizontalLine*> CPdfToExcel::CollectHorizontalLines(size_t nPage)
{
    vector<processor::types::HorizontalLine*> retval;
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


struct Rectangle
{
    int ll_x;
    int ll_y;
    int ur_x;
    int ur_y;

    void FromString(const string& text)
    {
        auto bb = split_on_space(text);
        ll_x = atoi(bb[0].c_str());
        ll_y = atoi(bb[1].c_str());
        ur_x = atoi(bb[2].c_str());
        ur_y = atoi(bb[3].c_str());
    }
};

const processor::types::HorizontalLine* CPdfToExcel::DoesTextHaveAUnderline(const processor::pdf::types::TextAtLocation* text, const vector<processor::types::HorizontalLine*>& lines, const struct Rectangle* mb)
{
    (void)mb;
    //if ll_y is not 0 then...
    double Tmy = text->GS.TextState.Tm.y;
    for (auto& line : lines) {
        double Cmy = line->y;
        if (Cmy < Tmy) {
            double diff = abs(Tmy - Cmy);
            if (diff < text->GS.TextState.Tfs) {
                return line;
            } else {
                return nullptr;
            }
        }
    }
    return nullptr;
}

vector<const processor::pdf::types::TextAtLocation*> GetThisPagesTextNodes(const vector<processor::pdf::types::TextAtLocation>& all_texts, size_t nPage)
{
    vector<const processor::pdf::types::TextAtLocation*> retval;
    for (auto& text : all_texts) {
        if (text.nPage == nPage) {
            retval.push_back(&text);
        }
    }
    return retval;
}


void CPdfToExcel::ProcessAllTextNodes(const vector<processor::pdf::types::TextAtLocation>& all_texts)
{
    //all the pages are here, so we can do multi page-trace backs and look forwards
    //find all begin header stuff
    vector<size_t> arDescrIndexs;
    if (TryFindAllDescription(all_texts, arDescrIndexs)) {

        auto find_last_item_hint = FindLineItemTotals(all_texts);

        for (size_t n = 0; n < arDescrIndexs.size(); n++) {
            auto first = arDescrIndexs[n];
            auto& descr = all_texts[first];
            auto last = all_texts.size();
            if ((n + 1) < arDescrIndexs.size()) {
                last = arDescrIndexs[n + 1];
            }

            auto find_footer_hint = FindPageText(all_texts, &descr);

            //Find all Header items sorted by X value
            auto header_items = GetAllHeaderItems(all_texts, &descr);
            auto h_lines = CollectHorizontalLines(descr.nPage);
            struct Rectangle mb;
            mb.FromString(m_State.media_boxes[descr.nPage]);


            //trace back looking for a trade
            m_State.trade = "";
            TraceBackForTrade(all_texts, first);

            auto trade = FindNextTradeItem(all_texts, first, last);
            if (trade) {
                m_State.trade = trim_all(trade->text.substr(7));
            }
            //Setup the columns
            SetupColumns(header_items);

            //Find Rows now
            size_t iLastCol = ~0UL;
            size_t nCurrentPage = descr.nPage;

            auto this_page_texts = GetThisPagesTextNodes(all_texts, nCurrentPage);

            double dLineSize = abs(descr.GS.TextState.Tfs * descr.GS.TextState.Tm.d) + 2.0;
            double dFirstLine = descr.GS.TextState.Tm.y;
            double dLastLine = dFirstLine - dLineSize;

            for (size_t row = first; row != last; ++row) {
                auto& text_row = all_texts[row];

                auto trm = text_row.GS.GetUserSpaceMatrix();
                Matrix last_trm;
                if (row > 0) {
                    last_trm = all_texts[row - 1].GS.GetUserSpaceMatrix();
                }

                auto underlined = DoesTextHaveAUnderline(&text_row, h_lines, &mb);
                double dCurrentLine = text_row.GS.TextState.Tm.y;
                auto dLineDiff = abs(dLastLine - dCurrentLine);
                if (dLineDiff > (dLineSize * 2.0)) {
                    if (m_State.Row.empty() == false) {
                        if (m_State.Row[0].empty() == false && is_no_data_last_row(m_State.Row) == false) {
                            size_t iTbl = static_cast<size_t>(match_columns(m_State.columns, m_State.tables, m_State.locked_trade));
                            auto& tbl = m_State.tables[iTbl];
                            tbl.rows.emplace_back(std::move(m_State.Row));
                            m_State.iRow++;
                        }
                    }
                    break;
                }
                if (underlined) {
                    dLastLine = underlined->y - (dCurrentLine - underlined->y);
                } else {
                    dLastLine = dCurrentLine;
                }
                if (text_row.nPage != nCurrentPage) {
                    nCurrentPage = text_row.nPage;
                    find_footer_hint = FindPageText(all_texts, &text_row);
                }
                if (find_footer_hint) {
                    auto fh_y = find_footer_hint->GS.GetUserSpaceMatrix();
                    if (trm.y >= fh_y.y) {
                        m_State.Row.clear();
                        continue;
                    }
                }
                if (find_last_item_hint) {
                    if (text_row.nPage > find_last_item_hint->nPage) {
                        break;
                    }
                    auto fh_y = find_last_item_hint->GS.TextState.Tm.y;
                    auto test_y = text_row.GS.TextState.Tm.y;
                    if (text_row.nPage == find_last_item_hint->nPage && test_y < fh_y) {
                        break;
                    }
                }
                ProcessRows(all_texts[row], iLastCol);
            }
        }
    }
}


//We need to see if several ways are the best?
#if 0
//assume, Col 0 is Left Aligned, and Rest are Right Aligned.
static inline void ProcessCustomTextProcessor(const TextState& ts, string text)
{
    auto rect = Measure(ts, text);
    static int iLastCol = -1;
    if (text == "DESCRIPTION" || text == "Description") {
        auto test1 = CustomState.FindClosestToTopLeft(ts.Tm.x, ts.Tm.y);
        auto test2 = CustomState.FindClosestToTopRight(ts.Tm.x, ts.Tm.y);

        PdfRect maybe_rect{ 0 };
        bool has_maybe_rect = false;
        bool is_centered = false;
        if (test1 && test2) {
            maybe_rect.x = test1->x;
            maybe_rect.y = test1->y;
            maybe_rect.width = test2->x - test1->x;
            maybe_rect.height = 0;
            has_maybe_rect = true;
            double dMidText = rect.x + (rect.width / 2.0);
            double dMidMaybe = maybe_rect.x + (maybe_rect.width / 2.0);
            is_centered = abs(dMidText - dMidMaybe) < 8.0;
        }

        CustomState.iRow = 0;
        iLastCol = -1;
        CustomState.locked_trade = CustomState.trade;
        if (CustomState.locked_trade.find("CONTINUED -") != string::npos) {
            CustomState.locked_trade = CustomState.locked_trade.substr(12);
        }
        CustomState.columns.clear();
        CustomState.dHeaderY = rect.y;
        CustomState.columns.emplace_back(PdfColumn{ rect, maybe_rect, has_maybe_rect, is_centered, text });
        CustomState.Row.clear();
        CustomState.Row.resize(CustomState.columns.size());
    } else if (rect.y == CustomState.dHeaderY) {
        auto test1 = CustomState.FindClosestToTopLeft(ts.Tm.x, ts.Tm.y);
        auto test2 = CustomState.FindClosestToTopRight(ts.Tm.x, ts.Tm.y);

        PdfRect maybe_rect{ 0 };
        bool has_maybe_rect = false;
        bool is_centered = false;
        if (test1 && test2) {
            maybe_rect.x = test1->x;
            maybe_rect.y = test1->y;
            maybe_rect.width = test2->x - test1->x;
            maybe_rect.height = 0;
            has_maybe_rect = true;
            double dMidText = rect.x + (rect.width / 2.0);
            double dMidMaybe = maybe_rect.x + (maybe_rect.width / 2.0);
            is_centered = abs(dMidText - dMidMaybe) < 8.0;
        }
        CustomState.columns.emplace_back(PdfColumn{ rect, maybe_rect, has_maybe_rect, is_centered, text });
        std::sort(CustomState.columns.begin(), CustomState.columns.end(), [&](const auto&left, const auto&right) {
            return left.rect.x < right.rect.x;
        });
        CustomState.Row.resize(CustomState.columns.size());
    } else {
        if (ts.Tf.font->m_sBaseFontName.find("Bold") != string::npos) {
            CustomState.trade = text;
        }
        if (CustomState.columns.empty()) {
        } else {
            //find the table...
            int iTbl = match_columns(CustomState.columns, CustomState.tables, CustomState.locked_trade);
            auto& tbl = CustomState.tables[iTbl];
            //check to see if its text or a number
            if (is_number(text)) {
                //right aligned
                int iCol = 0;
                for (auto& col : CustomState.columns) {
                    bool bMatched = false;
                    if (col.has_maybe_rect) {
                        bMatched = (rect.x > col.maybe_rect.x) && (rect.x < (col.maybe_rect.x + col.maybe_rect.width));
                    } else {
                        auto fuzzy_left = iCol == 0 ? (col.rect.x - 5.0) : (CustomState.columns[iCol - 1].rect.x + CustomState.columns[iCol - 1].rect.width);// ;
                        auto fuzzy_right = col.rect.x + col.rect.width + 5.0;
                        bMatched = rect.x >= fuzzy_left && rect.x <= fuzzy_right &&
                            (rect.x + rect.width) >= fuzzy_left && (rect.x + rect.width) <= fuzzy_right;
                    }
                    //lets be a litte more skeptical about the columns
                    if (bMatched) {
                        /*int look = 0;
                        }

                        auto diff = abs((col.rect.x + col.rect.width) - (rect.x + rect.width));
                        if (diff < 10.0) {*/
                        if (iCol < iLastCol) {
                            tbl.rows.emplace_back(std::move(CustomState.Row));
                            CustomState.iRow++;
                            //printf("End of row because looks like column index is less.\n");
                        }
                        //printf("Col [%g,%d], %s\n", rect.y, iCol, parameters[0].c_str());
                        CustomState.Row.resize(CustomState.columns.size());
                        CustomState.Row[iCol] += text;
                        if (iCol == (CustomState.columns.size() - 1)) {
                            // printf("End of row because the data is targeting last col.\n");
                            tbl.rows.emplace_back(std::move(CustomState.Row));
                            CustomState.iRow++;
                            iLastCol = -1;
                        } else {
                            iLastCol = iCol;
                        }
                        break;
                    }
                    iCol++;
                }
            } else {
                int iCol = 0;
                for (auto& col : CustomState.columns) {
                    bool bMatched = false;
                    if (col.has_maybe_rect) {
                        bMatched = (rect.x > col.maybe_rect.x) && (rect.x < (col.maybe_rect.x + col.maybe_rect.width));
                    } else {
                        bMatched = col.rect.x == rect.x;
                    }
                    if (bMatched) {
                        if (iCol < iLastCol) {
                            //possible end of row
                            tbl.rows.emplace_back(std::move(CustomState.Row));
                            CustomState.iRow++;
                            // printf("End of row.\n");
                        }
                        //printf("Col [%g,%d], %s\n", rect.y, iCol, parameters[0].c_str());
                        CustomState.Row.resize(CustomState.columns.size());
                        if (CustomState.Row[iCol].empty() == false) {
                            if (CustomState.iRow == 0 && iCol == 0 && is_all_upper_case(CustomState.Row[iCol])) {
                                tbl.coverage = CustomState.Row[iCol];
                                CustomState.Row[iCol] = "";
                            } else if (iLastCol == 0 && is_no_data_last_row(CustomState.Row)) {
                                if (tbl.rows.empty() == false) {
                                    auto& lr = tbl.rows.back();
                                    lr[iLastCol] += " " + CustomState.Row[iCol];
                                    CustomState.Row[iCol] = "";
                                } else {
                                    CustomState.Row[iCol] += " ";;
                                }
                            } else {
                                CustomState.Row[iCol] += "\n";
                            }
                        }
                        CustomState.Row[iCol] += text;
                        if (iCol == (CustomState.columns.size() - 1)) {
                            //printf("End of row because the data is targeting last col.\n");
                            tbl.rows.emplace_back(std::move(CustomState.Row));
                            CustomState.iRow++;
                            iLastCol = -1;
                        } else {
                            iLastCol = iCol;
                        }
                        break;
                    } else if (iCol > 0) {
                        auto fuzzy_left = (CustomState.columns[iCol - 1].rect.x + CustomState.columns[iCol - 1].rect.width) - 8.0;// ;
                        auto fuzzy_right = col.rect.x + col.rect.width + 8.0;

                        if (rect.x >= fuzzy_left && rect.x <= fuzzy_right &&
                            (rect.x + rect.width) >= fuzzy_left && (rect.x + rect.width) <= fuzzy_right) {
                            if (iCol < iLastCol) {
                                tbl.rows.emplace_back(std::move(CustomState.Row));
                                CustomState.iRow++;
                                //printf("End of row because looks like column index is less.\n");
                            }
                            //printf("Col [%g,%d], %s\n", rect.y, iCol, parameters[0].c_str());
                            CustomState.Row.resize(CustomState.columns.size());
                            CustomState.Row[iCol] += text;
                            if (iCol == (CustomState.columns.size() - 1)) {
                                // printf("End of row because the data is targeting last col.\n");
                                tbl.rows.emplace_back(std::move(CustomState.Row));
                                CustomState.iRow++;
                                iLastCol = -1;
                            } else {
                                iLastCol = iCol;
                            }
                            break;
                        }
                    }
                    iCol++;
                }
            }
        }
    }
}
#endif


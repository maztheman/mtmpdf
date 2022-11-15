#include "stdafx.h"
#include "PdfXRef.h"

#include <numeric>
#include <algorithm>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfObjectStream.h"

#include "tools.h"
#include "zlib_helper.h"

using namespace std;

CPdfXRef::CPdfXRef(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfXRef::~CPdfXRef()
{
}

CPdfXRef* CPdfXRef::FromDictionary(CPdfDictionary* pDict, char* data, char* end_data)
{
    auto pDoc = pDict->m_pDocument;
    auto pObject = new CPdfXRef(pDoc);
    pObject->m_pDictionary = pDict;
    auto filter = pDict->GetProperty("Filter");
    if (filter && *filter == "FlateDecode") {
        auto length = pDict->GetProperty("Length");
        auto len = atoi(length->c_str());
        auto current = preview_next_line(data);
        if (current.find("stream") != string::npos) {
            get_next_line(&data);
            data = skip_space(data);
        } else {
            data = skip_space(data);
        }
        vector<char> buffer(len);
        memcpy(&buffer[0], data, len);
        vector<char> dest;
        InflateData(buffer, dest);

        auto pIndex = pDict->GetProperty("Index");
        vector<int> index_data;
        if (pIndex) {
            auto ii = split_on_space(pIndex->c_str());
            for (size_t n = 0; n < ii.size(); n += 2) {
                int i_obj = atoi(ii[n].c_str());
                int c_obj = atoi(ii[n + 1].c_str());
                for (int j = i_obj; j < (i_obj + c_obj); j++) {
                    index_data.push_back(j);
                }
            }
        } else {
            int i_obj = 0;
            int c_obj = atoi(pDict->GetProperty("Size")->c_str());
            for (int j = i_obj; j <= (i_obj + c_obj); j++) {
                index_data.push_back(j);
            }
        }

        size_t max_si = atoi(pDict->GetProperty("Size")->c_str()) + 1;

        pDoc->EnsureChapterSize(max_si);

        auto pW = pDict->GetProperty("W");
        auto split_W = split_on_space(pW->c_str());
        std::vector<int> W_sizes;
        for (auto& sw : split_W) {
            W_sizes.push_back(atoi(sw.c_str()));
        }

        size_t byte_row = accumulate(W_sizes.begin(), W_sizes.end(), 0);
        int i_obj = 0;
        for (size_t idx = 0; idx < dest.size(); idx += byte_row, i_obj++) {
            uint32_t offs = idx;
            uint32_t type = read_dynamic_size(&dest[0] + offs, W_sizes[0]);
            offs += W_sizes[0];
            if (type == 0) {
                uint32_t free_obj = read_dynamic_size(&dest[0] + offs, W_sizes[1]);
                offs += W_sizes[1];
                uint32_t free_gen = read_dynamic_size(&dest[0] + offs, W_sizes[2]);
                int yy = 0;
            } else if (type == 1) {
                uint32_t obj_byte_offset = read_dynamic_size(&dest[0] + offs, W_sizes[1]);;
                offs += W_sizes[1];
                uint32_t obj_gen = read_dynamic_size(&dest[0] + offs, W_sizes[2]);
                pDoc->SetChapterIndex(index_data[i_obj], obj_byte_offset);
            } else if (type == 2) {
                uint32_t obj_stream = read_dynamic_size(&dest[0] + offs, W_sizes[1]);
                offs += W_sizes[1];
                uint32_t obj_idx = read_dynamic_size(&dest[0] + offs, W_sizes[2]);
                pObject->m_ZipObjects.emplace_back(SObjectIndexLookup{ obj_stream , obj_idx , static_cast<uint32_t>(index_data[i_obj])});
                int yy = 0;
            } else {

            }
        }

        for (auto& o : pObject->m_ZipObjects) {
            pDoc->ProcessObject(o.obj_stream);
            CPdfObjectStream* pStreamObject = static_cast<CPdfObjectStream*>(pDoc->GetObjectData(o.obj_stream));
            pStreamObject->ProcessObject(o.obj_stream_index);
        }

        return pObject;
    }

    return nullptr;
}



//old ass code
#if 0


//Tf gets moved into BT please


auto texts = split_bt(content);

std::vector<std::string> Row;
string trade;
string locked_trade;
int iRow = 0;
for (auto& text : texts) {
    auto parts = split_bt_parts(text);
    vector<string> parameters;
    for (auto& p : parts) {
        if (p == "RG") {
            parameters.clear();
        } else if (p == "rg") {
            parameters.clear();
        } else if (p.size() == 2 && p[0] == 'T') {
            switch (p[1]) {
                case 'f':
                {
                    auto fnt = parameters[0].substr(1);
                    ts.Tf.font = FontMaps[fnt];
                    ts.Tf.size = atof(parameters[1].c_str());
                }
                break;
                case 'm':
                {
                    ts.Tm.a = atof(parameters[0].c_str());
                    ts.Tm.b = atof(parameters[1].c_str());
                    ts.Tm.c = atof(parameters[2].c_str());
                    ts.Tm.d = atof(parameters[3].c_str());
                    ts.Tm.x = parameters[4];
                    ts.Tm.y = parameters[5];
                }
                break;
                case 'J':
                case 'j':
                {

                    auto rect = Measure(ts, parameters[0]);
                    static int iLastCol = -1;
                    if (parameters[0] == "DESCRIPTION" || parameters[0] == "Description") {
                        iRow = 0;
                        iLastCol = -1;
                        locked_trade = trade;
                        if (locked_trade.find("CONTINUED -") != string::npos) {
                            locked_trade = locked_trade.substr(12);
                        }
                        columns.clear();
                        dHeaderY = rect.y;
                        columns.emplace_back(PdfColumn{ rect, parameters[0] });
                        Row.clear();
                        Row.resize(columns.size());
                    } else if (rect.y == dHeaderY) {
                        columns.emplace_back(PdfColumn{ rect, parameters[0] });
                        std::sort(columns.begin(), columns.end(), [&](const auto&left, const auto&right) {
                            return left.rect.x < right.rect.x;
                        });
                        Row.resize(columns.size());
                    } else {
                        if (ts.Tf.font->m_sBaseFontName.find("Bold") != string::npos) {
                            trade = parameters[0];
                        }
                        if (columns.empty()) {
                        } else {
                            //find the table...
                            int iTbl = match_columns(columns, tables, locked_trade);
                            auto& tbl = tables[iTbl];
                            //check to see if its text or a number
                            if (is_number(parameters[0])) {
                                //right aligned
                                int iCol = 0;
                                for (auto& col : columns) {


                                    //lets be a litte more skeptical about the columns

                                    auto fuzzy_left = iCol == 0 ? (col.rect.x - 5.0) : (columns[iCol - 1].rect.x + columns[iCol - 1].rect.width);// ;
                                    auto fuzzy_right = col.rect.x + col.rect.width + 5.0;

                                    if (rect.x >= fuzzy_left && rect.x <= fuzzy_right &&
                                        (rect.x + rect.width) >= fuzzy_left && (rect.x + rect.width) <= fuzzy_right) {
                                        /*int look = 0;
                                        }

                                        auto diff = abs((col.rect.x + col.rect.width) - (rect.x + rect.width));
                                        if (diff < 10.0) {*/
                                        if (iCol < iLastCol) {
                                            tbl.rows.emplace_back(std::move(Row));
                                            iRow++;
                                            //printf("End of row because looks like column index is less.\n");
                                        }
                                        //printf("Col [%g,%d], %s\n", rect.y, iCol, parameters[0].c_str());
                                        Row.resize(columns.size());
                                        Row[iCol] += parameters[0];
                                        if (iCol == (columns.size() - 1)) {
                                            // printf("End of row because the data is targeting last col.\n");
                                            tbl.rows.emplace_back(std::move(Row));
                                            iRow++;
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
                                for (auto& col : columns) {
                                    if (col.rect.x == rect.x) {
                                        if (iCol < iLastCol) {
                                            //possible end of row
                                            tbl.rows.emplace_back(std::move(Row));
                                            iRow++;
                                            // printf("End of row.\n");
                                        }
                                        //printf("Col [%g,%d], %s\n", rect.y, iCol, parameters[0].c_str());
                                        Row.resize(columns.size());
                                        if (Row[iCol].empty() == false) {
                                            if (iRow == 0 && iCol == 0 && is_all_upper_case(Row[iCol])) {
                                                tbl.coverage = Row[iCol];
                                                Row[iCol] = "";
                                            } else if (iLastCol == 0 && is_no_data_last_row(Row)) {
                                                if (tbl.rows.empty() == false) {
                                                    auto& lr = tbl.rows.back();
                                                    lr[iLastCol] += " " + Row[iCol];
                                                    Row[iCol] = "";
                                                }
                                            } else {
                                                Row[iCol] += "\n";
                                            }
                                        }
                                        Row[iCol] += parameters[0];
                                        if (iCol == (columns.size() - 1)) {
                                            //printf("End of row because the data is targeting last col.\n");
                                            tbl.rows.emplace_back(std::move(Row));
                                            iRow++;
                                            iLastCol = -1;
                                        } else {
                                            iLastCol = iCol;
                                        }
                                        break;
                                    } else if (iCol > 0) {
                                        auto fuzzy_left = (columns[iCol - 1].rect.x + columns[iCol - 1].rect.width) - 8.0;// ;
                                        auto fuzzy_right = col.rect.x + col.rect.width + 8.0;

                                        if (rect.x >= fuzzy_left && rect.x <= fuzzy_right &&
                                            (rect.x + rect.width) >= fuzzy_left && (rect.x + rect.width) <= fuzzy_right) {
                                            if (iCol < iLastCol) {
                                                tbl.rows.emplace_back(std::move(Row));
                                                iRow++;
                                                //printf("End of row because looks like column index is less.\n");
                                            }
                                            //printf("Col [%g,%d], %s\n", rect.y, iCol, parameters[0].c_str());
                                            Row.resize(columns.size());
                                            Row[iCol] += parameters[0];
                                            if (iCol == (columns.size() - 1)) {
                                                // printf("End of row because the data is targeting last col.\n");
                                                tbl.rows.emplace_back(std::move(Row));
                                                iRow++;
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
                break;
                default:
                    break;
            }
            parameters.clear();
        } else {
            parameters.push_back(p);
        }

    }
}
int look = 0;
}
    }

#endif

#include "PdfXRef.h"

#include <numeric>
#include <algorithm>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfObjectStream.h"

#include "pdf/utils/tools.h"
#include "pdf/utils/zlib_helper.h"

using namespace std;

CPdfXRef::CPdfXRef(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CPdfXRef::~CPdfXRef()
{
}

/*
* 
* The Paeth filter computes a simple linear function of the three neighboring pixels (left, above, upper left), then chooses as predictor the neighboring pixel closest to the computed value. This technique is due to Alan W. Paeth [PAETH].

To compute the Paeth filter, apply the following formula to each byte of the scanline:

   Paeth(x) = Raw(x) - PaethPredictor(Raw(x-bpp), Prior(x),
                                      Prior(x-bpp))

where x ranges from zero to the number of bytes representing the scanline minus one, Raw(x) refers to the raw data byte at that byte position in the scanline, Prior(x) refers to the unfiltered bytes of the prior scanline, and bpp is defined as for the Sub filter.

Note this is done for each byte, regardless of bit depth. Unsigned arithmetic modulo 256 is used, so that both the inputs and outputs fit into bytes. The sequence of Paeth values is transmitted as the filtered scanline.

The PaethPredictor function is defined by the following pseudocode: 

function PaethPredictor (a, b, c)
   begin
        ; a = left, b = above, c = upper left
        p := a + b - c        ; initial estimate
        pa := abs(p - a)      ; distances to a, b, c
        pb := abs(p - b)
        pc := abs(p - c)
        ; return nearest of a,b,c,
        ; breaking ties in order a,b,c.
        if pa <= pb AND pa <= pc then return a
        else if pb <= pc then return b
        else return c
   end
*/

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

        int predictor = 0;
        int columns = 0;

        if (auto pDecodeParms = pDict->GetDictionary("DecodeParms"); pDecodeParms) {
            if (auto pPredictor = pDecodeParms->GetProperty("Predictor"); pPredictor) {
                predictor = atoi(pPredictor->c_str());
            }
            if (auto pColumns = pDecodeParms->GetProperty("Columns"); pColumns) {
                columns = atoi(pColumns->c_str());
            }
        }

        auto pW = pDict->GetProperty("W");
        auto split_W = split_on_space(pW->c_str());
        std::vector<int> W_sizes;
        for (auto& sw : split_W) {
            W_sizes.push_back(atoi(sw.c_str()));
        }

        size_t byte_row = std::accumulate(W_sizes.begin(), W_sizes.end(), 0) + ((predictor >= 10) ? 1 : 0);

        int c_row_num = dest.size() / byte_row;

        auto pIndex = pDict->GetProperty("Index");
        vector<int> index_data;

        int start_offset = 0;

        if (pIndex) {
            auto ii = split_on_space(pIndex->c_str());
            for (size_t n = 0; n < ii.size(); n += 2) {
                int i_obj = atoi(ii[n].c_str());
                int c_obj = atoi(ii[n + 1].c_str());

                /*start_offset = i_obj;

                if ((i_obj + c_obj) > (int)index_data.size()) {
                    index_data.resize(i_obj + c_obj);
                    std::iota(index_data.begin(), index_data.end(), 0);
                }*/

                for (int j = i_obj; j < (i_obj + c_obj); j++) {
                    index_data.push_back(j);
                }
            }
        } else {
            int i_obj = 0;
            int c_obj = atoi(pDict->GetProperty("Size")->c_str());

            //what is bigger Size or actual size / byte row
          

            c_obj = (std::max)(c_row_num, c_obj);

            for (int j = i_obj; j <= (i_obj + c_obj); j++) {
                index_data.push_back(j);
            }
        }

        size_t max_si = (std::max)(c_row_num, atoi(pDict->GetProperty("Size")->c_str())) + 1;

        pDoc->EnsureChapterSize(max_si);

        
        std::vector<std::vector<char>> rows;
        rows.reserve(c_row_num);
        for (auto first = dest.begin(); first != dest.end(); first += byte_row) {
            std::vector<char> row;
            row.insert(row.begin(), first, first + byte_row);
            rows.emplace_back(std::move(row));
        }

        if (predictor >= 10) {
            std::vector<std::vector<char>> results;
            std::vector<char> previous;
            for (auto& row : rows) {
                std::vector<char> result;
                if (row[0] == 2) {
                    if (previous.empty()) {
                        result.insert(result.begin(), row.begin() + 1, row.end());
                    } else {
                        for (auto it_prev = previous.begin(), it_now = row.begin() + 1; it_now != row.end(); ++it_prev, ++it_now) {
                            result.push_back((*it_prev + *it_now) & 255);
                        }
                    }
                }
                previous = result;//copy
                results.emplace_back(std::move(result));
            }

            rows = results;
        }

        int i_obj = 0;
        for (auto& row : rows) {
            uint32_t offs = 0;
            uint32_t type = read_dynamic_size(&row[0] + offs, W_sizes[0]);
            offs += W_sizes[0];
            if (type == 0) {
                uint32_t free_obj = read_dynamic_size(&row[0] + offs, W_sizes[1]);
                offs += W_sizes[1];
                uint32_t free_gen = read_dynamic_size(&row[0] + offs, W_sizes[2]);
                pDoc->SetChapterIndex(index_data[i_obj], free_obj, free_gen, 'f');
            } else if (type == 1) {
                uint32_t obj_byte_offset = read_dynamic_size(&row[0] + offs, W_sizes[1]);;
                offs += W_sizes[1];
                uint32_t obj_gen = read_dynamic_size(&row[0] + offs, W_sizes[2]);
                pDoc->SetChapterIndex(index_data[i_obj], obj_byte_offset, obj_gen, 'n');
            } else if (type == 2) {
                uint32_t obj_stream = read_dynamic_size(&row[0] + offs, W_sizes[1]);
                offs += W_sizes[1];
                uint32_t obj_idx = read_dynamic_size(&row[0] + offs, W_sizes[2]);
                pObject->m_ZipObjects.emplace_back(SObjectIndexLookup{ obj_stream , obj_idx , i_obj >= (int)index_data.size() ? static_cast<uint32_t>(i_obj) : static_cast<uint32_t>(index_data[i_obj]) });
            }
            i_obj++;
        }

        /*int i_obj = 0;
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
                pDoc->SetChapterIndex(start_offset + index_data[i_obj], obj_byte_offset);
            } else if (type == 2) {
                uint32_t obj_stream = read_dynamic_size(&dest[0] + offs, W_sizes[1]);
                offs += W_sizes[1];
                uint32_t obj_idx = read_dynamic_size(&dest[0] + offs, W_sizes[2]);
                pObject->m_ZipObjects.emplace_back(SObjectIndexLookup{ obj_stream , obj_idx , i_obj >= (int)index_data.size() ? static_cast<uint32_t>(i_obj) :  static_cast<uint32_t>(index_data[i_obj])});
                int yy = 0;
            } else {

            }
        }*/

        auto pEncrypt = pDict->GetProperty("Encrypt");
        if (pEncrypt) {
            int obj = ObjectFromReference(pEncrypt);
            pDoc->ProcessObject(obj);
            int look = 0;
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

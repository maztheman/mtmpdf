#include <cassert>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <numeric>

#include "PdfDocument.h"
#include "PdfObject.h"
#include "PdfDictionary.h"
#include "PdfCatalog.h"
#include "PdfPages.h"
#include "PdfPage.h"
#include "PdfFont.h"
#include "PdfString.h"
#include "PdfObjectStream.h"
#include "PdfXRef.h"
#include "PdfFontDescriptor.h"

#include "ToUnicode.h"

#include "tools.h"
#include "zlib_helper.h"

using namespace std;

static char* skip_space_r(char* data)
{
    for (; *data == '\r' || *data == '\n'; data--);
    return data;
}


static char* find_space_r(char* data)
{
    for (; *data != '\r' && *data != '\n'; data--);
    return data;
}

static string get_prev_line(char** data)
{
    *data = skip_space_r(*data);
    auto prev_space = find_space_r(*data);
    string retval(prev_space + 1, *data + 1);
    *data = prev_space;
    return retval;
}


/*
static string preview_next_line(char* data)
{
    data = skip_space(data);
    auto nl = find_space(data);
    string retval(data, nl);
    return retval;
}
*/

static inline string HexToString_Ascii(string hex_string)
{
    string temp;
    for (size_t first = 0; first < hex_string.size(); first += 2) {
        string hex_char = hex_string.substr(first, 2);
        //0045
        char cc = 0;
        for (int i = 0; i < 2; i++) {
            char t = hex_char[i];
            if (t >= '0' && t <= '9') {
                cc |= (t - '0') << ((1 - i) * 4);
            } else if (t >= 'a' && t <= 'f') {
                cc |= 10 + ((t - 'a') << ((1 - i) * 4));
            } else if (t >= 'A' && t <= 'F') {
                cc |= 10 + ((t - 'A') << ((1 - i) * 4));
            }
        }
        temp += cc;
    }
    return temp;
}

static inline string HexToString_Unicode(CToUnicode* to_unicode, string hex_string)
{
    string retval;
    wstring temp;

    for (size_t first = 0; first < hex_string.size(); first += 4) {
        string hex_char = hex_string.substr(first, 4);
        //0045
        wchar_t cc = 0;
        for (int i = 0; i < 4; i++) {
            char t = hex_char[i];
            if (t >= '0' && t <= '9') {
                cc |= (t - '0') << ((3 - i) * 4);
            } else if (t >= 'a' && t <= 'f') {
                cc |= 10 + ((t - 'a') << ((3 - i) * 4));
            } else if (t >= 'A' && t <= 'F') {
                cc |= 10 + ((t - 'A') << ((3 - i) * 4));
            }
        }
        temp += to_unicode->GetMappedGlyph(cc);
    }
    return string(temp.begin(), temp.end());
}
//Text Array
static inline vector<TextAtLocation> ProcessOperatorTJ(DIGraphicsState& GS, const string& text, size_t nPage)
{
    auto& TS = GS.TextState;
    auto ToUnicode = TS.Tfont->m_pToUnicode;
    const char* first = &text[0];
    const auto last = &text[text.size()];
    string retval;
    const char* last_space = first;
    vector<TextAtLocation> texts;
    Matrix& tm = TS.Tm;
    CPdfFont* font = TS.Tfont;

    double dTextSpace = 0.0;
    //double dCurrentX = tm.x;
    //double dFirstX = dCurrentX;
    Matrix& mCurrent = tm;
    Matrix mFirst = mCurrent;


    for (; first != last; ++first) {
        if (*first == '[') {
            for (; first != last; ++first) {
                if (*first == ']') {
                    break;
                } else if (*first == '<') {
                    const char* hex_first = ++first;
                    for (; first != last; ++first) {
                        if (*first == '>') {
                            break;
                        }
                    }
                    string hex_string(hex_first, first);

                    //support these for real
                    double Tc = TS.Tc;
                    double Tw = TS.Tw;
                    double Th = TS.Th;
                    double Thr = (Th / 100.0);
                    double Tfs = TS.Tfs;
                    string temp;

                    if (ToUnicode) {
                        temp = HexToString_Unicode(ToUnicode, hex_string);
                    } else {
                        //its ascii mate, EASY!
                        temp = HexToString_Ascii(hex_string);
                    }
                    if (dTextSpace <= -478.0) {
                        auto copy_ts = GS;
                        copy_ts.TextState.Tm = mFirst;
                        texts.push_back({ copy_ts, retval, nPage });
                        retval.clear();
                        mFirst = mCurrent;
                    }

                    for (auto& c : temp) {
                        double gw = font->GetGlyphWidth(c);
                        double Tx = (((gw / 1000.0) - (dTextSpace / 1000.0)) * Tfs + Tc + Tw) * Thr;
                        //T(m) = T(tlm) * T(m)
                        Matrix Tupdate = Matrix::IdentityMatrix();
                        Tupdate.x = Tx;
                        Multiply(Tupdate, mCurrent);
                        mCurrent = Tupdate;
                        if (dTextSpace <= -478.0) {
                            mFirst = mCurrent;
                        }
                        retval += c;
                        dTextSpace = 0.0;
                    }
                    dTextSpace = 0.0;
                } else if (*first == '(') {
                    const char* tt = ++first;
                    vector<char> val;
                    for (; first != last; ++first) {
                        if (*first == ')') {
                            break;
                        } else if (*first == '\\') {
                            val.push_back(*(first + 1));
                            ++first;
                        } else {
                            val.push_back(*first);
                        }
                    }

                    //support these for real
                    double Tc = TS.Tc;
                    double Tw = TS.Tw;
                    double Th = TS.Th;
                    double Thr = Th / 100.0;
                    double Tfs = TS.Tfs;

                    string temp(val.begin(), val.end());

                    if (dTextSpace <= -478.0) {
                        auto copy_ts = GS;
                        copy_ts.TextState.Tm = mFirst;
                        texts.push_back({ copy_ts, retval, nPage });
                        retval.clear();
                        mFirst = mCurrent;
                    }

                    for (auto& c : temp) {
                        int ic = (unsigned char)c;
                        double gw = font->GetGlyphWidth(ic);
                        double Tx = (((gw / 1000.0) - (dTextSpace / 1000.0)) * Tfs + Tc + Tw) * Thr;
                        Matrix Tupdate = Matrix::IdentityMatrix();
                        Tupdate.x = Tx;
                        Multiply(Tupdate, mCurrent);
                        mCurrent = Tupdate;
                        if (dTextSpace <= -478.0) {
                            mFirst = mCurrent;
                        }
                        retval += c;
                        dTextSpace = 0.0;
                    }
                    dTextSpace = 0.0;

                } else if (*first == '-' || (*first >= '0' && *first <= '9')) {

                    const char* digit_first = first;
                    bool bNeg = false;
                    if (*first == '-') {
                        bNeg = true;
                        ++first;
                    }
                    for (; first != last; ++first) {
                        if (*first >= '0' && *first <= '9') {
                        } else if (*first == '.') {
                        } else {
                            break;
                        }
                    }

                    string numba(digit_first, first);
                    dTextSpace = atof(numba.c_str());
                    --first;
                }
            }
        }

        if (retval.empty() == false) {
            auto copy_ts = GS;
            copy_ts.TextState.Tm = mFirst;
            texts.push_back({ copy_ts, retval, nPage });
        }
    }

    return texts;
}



static inline string ProcessTextBlock(const DIGraphicsState& GS, const string& text)
{
    auto ToUnicode = GS.TextState.Tfont->m_pToUnicode;
    const char* first = &text[0];
    const auto last = &text[text.size()];

    //Tm is supposed to be updated as characters are shown. why dont i do that?

    string retval;

    const char* last_space = first;
    for (; first != last; ++first) {
        if (*first == '[') {
            for (; first != last; ++first) {
                if (*first == ']') {
                    break;
                } else if (*first == '<') {
                    const char* hex_first = ++first;
                    for (; first != last; ++first) {
                        if (*first == '>') {
                            break;
                        }
                    }
                    string hex_string(hex_first, first);
                    if (ToUnicode) {
                        retval += HexToString_Unicode(ToUnicode, hex_string);
                    } else {
                        //its ascii mate, EASY!
                        retval += HexToString_Ascii(hex_string);
                    }
                }
            }
        } else if (*first == '(') {
            //a "(, ), \" must have a \ before it
            const char* tt = ++first;
            vector<char> val;
            for (; first != last; ++first) {
                if (*first == ')') {
                    break;
                } else if (*first == '\\') {
                    val.push_back(*(first + 1));
                    ++first;
                } else {
                    val.push_back(*first);
                }
            }
            retval.append(val.begin(), val.end());
        } else if (*first == '<') {
            const char* hex_first = ++first;
            for (; first != last; ++first) {
                if (*first == '>') {
                    break;
                }
            }
            string hex_string(hex_first, first);
            if (ToUnicode) {
                retval += HexToString_Unicode(ToUnicode, hex_string);
            } else {
                //its ascii mate, EASY!
                retval += HexToString_Ascii(hex_string);
            }
        } else if (*first == '-' || (*first >= '0' && *first <= '9')) {

            const char* digit_first = first;
            bool bNeg = false;
            if (*first == '-') {
                bNeg = true;
                ++first;
            }
            for (; first != last; ++first) {
                if (*first >= '0' && *first <= '9') {
                } else {
                    break;
                }
            }

            string numba(digit_first, first);
            int num = atoi(numba.c_str());

            //move text a certain amount of pixels?  what the heck?

        }
    }
    return retval;
}

void CPdfDocument::ProcessCustomLine(const DIGraphicsState& GS, const PathState& PS, const PdfPoint& point, size_t nPage)
{
    if (PS.SubPath.size() < 2) { return; }
    
    const PdfPoint& last_point = PS.SubPath[PS.SubPath.size() - 2];

    if (point.x == last_point.x) {
        double lp_y = last_point.y;
        double y = point.y;
        //vertical line
        VerticalLine vert;
        vert.x = point.x;
        if (GS.CTM.d < 0.0) {
            vert.y = min(lp_y, y);
        } else {
            vert.y = max(lp_y, y);
        }
        vert.height = abs(y - lp_y);
        vert.nPage = nPage;
        vert.Cma = GS.CTM.a;
        m_AllVerticalLines.emplace_back(std::move(vert));
    } else if (point.y == last_point.y) {
        double lp_x = last_point.x;
        double x = point.x;
        HorizontalLine horz;
        if (GS.CTM.a < 0.0) {
            horz.x = max(lp_x, x);
        } else {
            horz.x = min(lp_x, x);
        }
        horz.y = point.y;
        horz.width = abs(x - lp_x);
        horz.nPage = nPage;
        horz.Cmd = GS.CTM.d;
        m_AllHorizontalLines.emplace_back(std::move(horz));
    }
}

CPdfDocument::CPdfDocument(std::string sFilename, std::vector<char>&& data)
    : m_Data(std::move(data))
    , m_sFilename(sFilename)
{
}

CPdfDocument::~CPdfDocument()
{
    for (auto& obj : m_AllObjects) {
        delete obj;
    }
}

void CPdfDocument::ProcessObject(int object)
{
    if (object == 17) {
        int g = 0;
    }

    auto sObject = to_string(object);
    if (m_ObjectData.find(sObject) != m_ObjectData.end()) {
        return;
    }
    auto offset = m_ChapterOffset[object];
    char* data = &m_Data[offset];
    auto line = get_next_line(&data);
    auto checker = ReadObject(line);
    assert(checker == object);
    auto pDict = CPdfDictionary::FromRawString(this, data, &m_Data[m_Data.size() - 1]);
    m_ObjectData.emplace(sObject, ProcessType(pDict, data));
}

void CPdfDocument::Initialize()
{
    //find EOF's
    size_t end = m_Data.size() - 1;
    char* begin = &m_Data[0];
    char* data = &m_Data[end];
    auto eof = get_prev_line(&data);
    if (eof == "%%EOF") {
        auto sXRefOffset = get_prev_line(&data);
        auto sXRefLabal = get_prev_line(&data);
        if (sXRefLabal == "startxref") {
            ReadXRef(atoi(sXRefOffset.c_str()));
        }
    }
    Process();
}

void CPdfDocument::Process()
{
    auto trailer = m_ObjectData.find("trailer");
    if (trailer == m_ObjectData.end()) {
        return;
    }
    if (trailer->second->GetType() == pdf_type::dictionary) {
        auto pDict = static_cast<CPdfDictionary*>(trailer->second);
        auto root = pDict->GetValue().find("Root");
        int root_obj = ObjectFromReference(root->second);
        ProcessObject(root_obj);
        m_ObjectData.emplace("root", m_ObjectData[to_string(root_obj)]);
    }

    /*
    int object = 0;
    for (auto& offset : m_ChapterOffset) {
    if (offset == 0) {
    object++;
    continue;
    }

    char* data = &m_Data[offset];
    auto line = get_next_line(&data);
    line = get_next_line(&data);
    auto pDict = CPdfDictionary::FromString(line);
    auto sObject = to_string(object);
    m_ObjectData.insert(make_pair(sObject, pDict));

    auto& dict = pDict->GetValue();
    auto d = dict.find("Filter");
    if (d != dict.end()) {
    if (*d->second == "FlateDecode") {
    auto cnt = dict.find("Length");
    if (cnt != dict.end()) {
    int len = atoi(cnt->second->c_str());
    if (line.find("stream") == string::npos) {
    line = get_next_line(&data);
    } else {
    data = skip_space(data);
    }
    vector<char> buffer(len);
    memcpy(&buffer[0], data, len);
    vector<char> dest;
    InflateData(buffer, dest);
    ProcessTextBlock(string(dest.begin(), dest.end()));
    int bb = 0;
    }
    }
    }

    object++;
    }*/
}



struct SIndexData
{
    int obj;
    int count;
};

void CPdfDocument::ReadXRef(int offset)
{
    bool bRootChapter = false;
    char* data = &m_Data[offset];
    auto sXRefLabel = get_next_line(&data);
    if (sXRefLabel == "xref") {
        for (;;) {
            auto line = get_next_line(&data);
            size_t start_object, count;
            {
                stringstream ss(line);
                ss >> start_object >> count;
            }

            if (start_object >= m_ChapterOffset.size()) {
                m_ChapterOffset.resize(start_object + count);
            }

            for (size_t i = 0; i < count; i++) {
                line = get_next_line(&data);
                int nChapterOffset, nChapterGeneration;
                string flag;
                {
                    stringstream ss(line);
                    ss >> nChapterOffset >> nChapterGeneration >> flag;
                }
                if (i == 0 && nChapterGeneration == 65535) {
                    bRootChapter = true;
                }
                m_ChapterOffset[start_object + i] = nChapterOffset;
            }
            auto trailer = preview_next_line(data);
            if (trailer.find("trailer") != string::npos) {
                break;
            }
        }
    } else {
        int obj = ReadObject(sXRefLabel);
        if (obj != -1) {
            ProcessInternalObject(obj, data, &m_Data[m_Data.size() - 1]);
            auto xref = static_cast<CPdfXRef*>(GetObjectData(obj));
            bRootChapter = xref->m_pDictionary->GetProperty("Prev") == nullptr;
        }
    }

    //if (bRootChapter == false) {
        auto line = get_next_line(&data);
        auto trailer_idx = line.find("trailer");
        if (trailer_idx != string::npos) {
            auto open_char = line.find("<<", trailer_idx);
            CPdfDictionary* dict = nullptr;
            if (open_char == string::npos) {
                dict = CPdfDictionary::FromRawString(this, data, &m_Data[m_Data.size() - 1]);
            } else {
               dict = CPdfDictionary::FromString(this, line.substr(open_char));
            }
            if (dict->GetProperty("Root") != nullptr) {
                m_ObjectData.emplace("trailer", dict);
            }
            auto prev = dict->GetValue().find("Prev");
            if (prev != dict->GetValue().end()) {
                ReadXRef(atoi(prev->second->c_str()));
            }
        }
    //}
}

void CPdfDocument::ProcessInternalObject(int object, char* data, char* end_data)
{
    auto sObject = to_string(object);
    CPdfDictionary* pDict = CPdfDictionary::FromRawString(this, data, end_data);
    m_ObjectData.emplace(sObject, ProcessType(pDict, data));
}

void CPdfDocument::EnsureChapterSize(size_t count)
{
    if (m_ChapterOffset.size() < count) {
        m_ChapterOffset.resize(count);
   }
}

CPdfObject* CPdfDocument::ProcessType(CPdfDictionary* pDict, char* data)
{
    auto& val = pDict->GetValue();
    auto type = val.find("Type");
    if (type == val.end()) {
        auto filter = pDict->GetProperty("Filter");
        if (filter && trim_all(filter->c_str()) == "FlateDecode") {
            auto length = pDict->GetProperty("Length");
            auto len_obj = ObjectFromReference(length);
            size_t len = 0;
            if (len_obj == -1) {
                len = atoi(length->c_str());
            } else {
                auto len_off = m_ChapterOffset[len_obj];
                auto len_data = &m_Data[len_off];
                auto line = get_next_line(&len_data);
                auto checker = ReadObject(line);
                assert(checker == len_obj);
                line = get_next_line(&len_data);
                len = atoi(line.c_str());
                int yy = 0;
            }

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
            return CPdfString::FromVector(this, dest);
        }
        return pDict;
    }
    if (*type->second == "Catalog") {
        return CPdfCatalog::FromDictionary(pDict);
    } else if (*type->second == "Pages") {
        return CPdfPages::FromDictionary(pDict);
    } else if (*type->second == "Page") {
        return CPdfPage::FromDictionary(pDict);
    } else if (*type->second == "Font") {
        return CPdfFont::FromDictionary(pDict);
    } else if (*type->second == "ObjStm") {
        return CPdfObjectStream::FromDictionary(pDict, data);
    } else if (*type->second == "XRef") {
        auto prev = pDict->GetValue().find("Prev");
        if (prev != pDict->GetValue().end()) {
            ReadXRef(atoi(prev->second->c_str()));
        } else {
            m_ObjectData.emplace("trailer", pDict);
        }
        return CPdfXRef::FromDictionary(pDict, data, &m_Data[m_Data.size() - 1]);
    } else if (*type->second == "FontDescriptor") {
        return CPdfFontDescriptor::FromDictionary(pDict);
    }

    return pDict;
}


CPdfObject* CPdfDocument::GetObjectData(int object)
{
    auto sObject = to_string(object);
    return m_ObjectData[sObject];
}

CPdfObject* CPdfDocument::GetObjectData(std::string object)
{
    return m_ObjectData[object];
}

void CPdfDocument::SetChapterIndex(size_t object, size_t offset)
{
    m_ChapterOffset[object] = offset;
}

void CPdfDocument::CollectAllTextNodes(std::function<bool(const TextAtLocation&, const TextAtLocation&)> sort_function)
{
    CPdfCatalog* pCatalog = static_cast<CPdfCatalog*>(GetObjectData("root"));
    // std::vector<PdfColumn> columns;
    // double dHeaderY = -99990.0;
    map<string, CPdfFont*> FontMaps;
    // vector<PdfTable> tables;
    pCatalog->Process();

    m_AllVerticalLines.clear();
    
    size_t nPage = 0;
    for (auto pPage : pCatalog->m_pPages->m_Data) {
        pPage->Process();
        auto media_box = pPage->m_pDictionary->GetProperty("MediaBox");
        if (media_box) {
            m_MediaBoxes.push_back(media_box->c_str());
        } else {
            m_MediaBoxes.push_back("");
        }
        for (auto& content : pPage->m_Contents) {
            FontMaps.clear();
            for (auto& f : pPage->m_Fonts) {
                FontMaps[f->m_sAlias] = f;
            }

            bool bEndOfValue = false;

            //pdf works like a queue, where things are pushed into the queue then executed
            char* first = &content[0];
            char* last = &content[content.size() - 1];
            DIGraphicsState GS;
            PathState       PS;


            GS.CTM = Matrix{1.0, 0.0, 0.0, 1.0, 0.0, 792.0};
            Multiply(GS.CTM, Matrix{1.0, 0.0, 0.0, -1.0, 0.0, 0.0});


            std::deque<DIGraphicsState> GraphicsStateQueue;
            std::vector<std::string> command_queue;

            char* last_space = first;
            for (; first != last; ++first) {
                if (*first == '[') {
                    for (; first != last; ++first) {
                        if (*first != ']') {
                        } else {
                            bEndOfValue = true;
                            break;
                        }
                    }
                } else if (*first == '(') {
                    for (; first != last; ++first) {
                        if (*first != ')') {
                            if (*first == '\\') {
                                ++first;
                            }
                        } else {
                            bEndOfValue = true;
                            break;
                        }
                    }
                } else if (*first == '<' && *(first + 1) == '<') {
                    for (; first != last; ++first) {
                        if (*first == '>' && *(first + 1) == '>') {
                            first += 2;
                            break;
                        }
                    }
                    if ((first - last_space) > 0) {
                        command_queue.emplace_back(last_space, first);
                    }
                    last_space = first;
                } else if (*first == '<') {
                    for (; first != last; ++first) {
                        if (*first != '>') {
                        } else {
                            bEndOfValue = true;
                            break;
                        }
                    }
                } else if (isspace(*first) || bEndOfValue) {
                    std::string value(last_space, first);
                    int oper = -1;
                    if (value.size() <= 2) {
                        if ((value[0] >= 'a' && value[0] <= 'z')) {
                            oper = (value[0] - 'a') & 31;
                        } else if ((value[0] >= 'A' && value[0] <= 'Z')) {
                            oper = ((value[0] - 'A') & 31) | 0x20;
                        }
                        if (oper != -1 && value.size() == 2) {
                            oper <<= 6;
                            if ((value[1] >= 'a' && value[1] <= 'z')) {
                                oper |= (value[1] - 'a');
                            } else if ((value[1] >= 'A' && value[1] <= 'Z')) {
                                oper |= (value[1] - 'A') | 0x20;
                            }
                        }
                    } else if (value.size() == 3) {
                        if ((value[0] >= 'a' && value[0] <= 'z')) {
                            oper = (value[0] - 'a') & 31;
                        } else if ((value[0] >= 'A' && value[0] <= 'Z')) {
                            oper = ((value[0] - 'A') & 31) | 0x20;
                        }
                        if (oper != -1) {
                            oper <<= 6;
                            if ((value[1] >= 'a' && value[1] <= 'z')) {
                                oper |= (value[1] - 'a');
                            } else if ((value[1] >= 'A' && value[1] <= 'Z')) {
                                oper |= (value[1] - 'A') | 0x20;
                            }
                        }
                        if (oper != -1) {
                            oper <<= 6;
                            if ((value[2] >= 'a' && value[2] <= 'z')) {
                                oper |= (value[2] - 'a');
                            } else if ((value[2] >= 'A' && value[2] <= 'Z')) {
                                oper |= (value[2] - 'A') | 0x20;
                            }
                        }
                    }

                    bool is_oper = false;
                    if (oper != -1) {
                        //12 bit data
                        if (oper <= 0x3F) {
                            //lower 6 bits
                            switch (oper & 0x3F) {
                                case 0x21:
                                case 0x01:
                                    //b
                                    if (PS.SubPath.empty() == false) {
                                        PS.SubPath.push_back(PS.SubPath.front());
                                        //because its full, its probably not a line! ??
                                        ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                        PS.Path.emplace_back(std::move(PS.SubPath));
                                    }
                                    //Fill Path
                                    //Clear Path
                                    PS.Path.clear();
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x03:
                                    //d
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x25:
                                case 0x05:
                                {
                                    //f
                                    if (PS.SubPath.empty() == false) {
                                        PS.SubPath.push_back(PS.SubPath.front());
                                        ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                        PS.Path.emplace_back(std::move(PS.SubPath));
                                    }
                                    //Fill Path
                                    //Clear Path
                                    PS.Path.clear();
                                    is_oper = true;
                                    command_queue.clear();
                                }
                                    break;
                                case 0x06:
                                    //g
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x07:
                                    //h
                                    if (PS.SubPath.empty() == false) {
                                        PS.SubPath.push_back(PS.SubPath.front());
                                        ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                        PS.Path.emplace_back(std::move(PS.SubPath));
                                    }
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x0B:
                                    //l
                                    PS.SubPath.emplace_back(atof(command_queue[0].c_str()), atof(command_queue[1].c_str()));
                                    ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x0C:
                                    //m
                                    PS.SubPath.clear();
                                    PS.SubPath.emplace_back(atof(command_queue[0].c_str()), atof(command_queue[1].c_str()));
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x0D:
                                    //n
                                    is_oper = true;
                                    PS.SubPath.clear();
                                    PS.Path.clear();//dunno if clear is appropriate but this isnt trying to print to screen, yet
                                    command_queue.clear();
                                    break;
                                case 0x10:
                                    //q push state?
                                    GraphicsStateQueue.push_back(GS);
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x12:
                                    //s
                                    if (PS.SubPath.empty() == false) {
                                        PS.SubPath.push_back(PS.SubPath.front());
                                        ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                        PS.Path.emplace_back(std::move(PS.SubPath));
                                    }
                                    //Stroke Path
                                    //Then Clear!?
                                    PS.Path.clear();
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x16:
                                    //w
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x26:
                                    //G
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x29:
                                    //J
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x2C:
                                    //M
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x30:
                                    //Q Pop state?
                                    GS = GraphicsStateQueue.back();
                                    GraphicsStateQueue.pop_back();
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x32:
                                    //S
                                    //Stroke path.
                                    //Then clear?
                                    PS.Path.clear();
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                                case 0x36:
                                    //W
                                    //Set Clipping path equal to PS.Path
                                    GS.ClippingPath.Path = PS.Path;
                                    GS.ClippingPath.Rule = 0;
                                    is_oper = true;
                                    command_queue.clear();
                                    break;
                            }
                        } else {
                            //cm, Do, T[f,j,J,n,z,], BT, ET
                            bool is_oper_partial = false;

                            int shifty = (value.size() - 1) * 6;

                            switch ((oper >> shifty) & 0x3F) {
                                case 0x01:
                                    //b
                                    is_oper_partial = true;
                                    break;
                                case 0x02:
                                    //c
                                    is_oper_partial = true;
                                    break;
                                case 0x05:
                                    //f
                                    is_oper_partial = true;
                                    break;
                                case 0x06:
                                    //g
                                    is_oper_partial = true;
                                    break;
                                case 0x11:
                                    //r
                                    is_oper_partial = true;
                                    break;
                                case 0x12:
                                    //s
                                    is_oper_partial = true;
                                    break;
                                case 0x21:
                                    //B
                                    is_oper_partial = true;
                                    break;
                                case 0x22:
                                    //C
                                    is_oper_partial = true;
                                    break;
                                case 0x23:
                                    //D
                                    is_oper_partial = true;
                                    break;
                                case 0x24:
                                    //E
                                    is_oper_partial = true;
                                    break;
                                case 0x31:
                                    //R
                                    is_oper_partial = true;
                                    break;
                                case 0x32:
                                    //S
                                    is_oper_partial = true;
                                    break;
                                case 0x33:
                                    //T
                                    is_oper_partial = true;
                                    break;
                                case 0x36:
                                    is_oper_partial = true;
                                    break;
                                default:
                                    printf("Unknown operator %s\n", value.c_str());
                                    break;
                            }

                            if (is_oper_partial) {
                                if (value == "cm") {
                                    Matrix new_cm;
                                    new_cm.a = atof(command_queue[0].c_str());
                                    new_cm.b = atof(command_queue[1].c_str());
                                    new_cm.c = atof(command_queue[2].c_str());
                                    new_cm.d = atof(command_queue[3].c_str());
                                    new_cm.x = atof(command_queue[4].c_str());
                                    new_cm.y = atof(command_queue[5].c_str());

                                    Multiply(GS.CTM, new_cm);

                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "b*") {
                                    if (PS.SubPath.empty() == false) {
                                        PS.SubPath.push_back(PS.SubPath.front());
                                        //because its full, its probably not a line! ??
                                        ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                        PS.Path.emplace_back(std::move(PS.SubPath));
                                    }
                                    PS.Path.clear();
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "f*") {
                                    if (PS.SubPath.empty() == false) {
                                        PS.SubPath.push_back(PS.SubPath.front());
                                        ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                        PS.Path.emplace_back(std::move(PS.SubPath));
                                    }
                                    //Fill Path Even-Odd Rule
                                    //Clear Path
                                    PS.Path.clear();
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "B*") {
                                    //Fill & Stroke Path
                                    //Clear Path
                                    PS.SubPath.clear();
                                    PS.Path.clear();
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "W*") {
                                    //Current Path to Clipping Path with Even-Odd Rule
                                    GS.ClippingPath.Path = PS.Path;
                                    GS.ClippingPath.Rule = 1;
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "re") {
                                    PS.SubPath.clear();
                                    double re_x = atof(command_queue[0].c_str());
                                    double re_y = atof(command_queue[1].c_str());
                                    double re_cx = atof(command_queue[2].c_str());
                                    double re_cy = atof(command_queue[3].c_str());
                                    PS.SubPath.emplace_back(re_x, re_y);
                                    PS.SubPath.emplace_back(re_x + re_cx, re_y);
                                    ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                    PS.SubPath.emplace_back(re_x + re_cx, re_y + re_cy);
                                    ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                    PS.SubPath.emplace_back(re_x, re_y + re_cy);
                                    ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                    PS.SubPath.emplace_back(re_x, re_y);
                                    ProcessCustomLine(GS, PS, PS.SubPath.back(), nPage);
                                    PS.Path.emplace_back(std::move(PS.SubPath));
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "CS") {
                                    GS.ColorSpace.Stroke = command_queue[0];
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "SC") {
                                    std::string joined;
                                    for (auto& op : command_queue) {
                                        joined += op + " ";
                                    }
                                    GS.Color.Stroke = trim_all(joined);
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "SCN") {
                                    std::string joined;
                                    for (auto& op : command_queue) {
                                        joined += op + " ";
                                    }
                                    GS.Color.Stroke = trim_all(joined);
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "EMC") {
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "BDC") {
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "RG") {
                                    GS.ColorSpace.Stroke = "/DeviceRGB";
                                    std::string joined;
                                    for (auto& op : command_queue) {
                                        joined += op + " ";
                                    }
                                    GS.Color.Stroke = trim_all(joined);
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "rg") {
                                    GS.ColorSpace.NonStroke = "/DeviceRGB";
                                    std::string joined;
                                    for (auto& op : command_queue) {
                                        joined += op + " ";
                                    }
                                    GS.Color.NonStroke = trim_all(joined);
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "cs") {
                                    GS.ColorSpace.NonStroke = command_queue[0];
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "sc") {
                                    std::string joined;
                                    for (auto& op : command_queue) {
                                        joined += op + " ";
                                    }
                                    GS.Color.NonStroke = trim_all(joined);
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "scn") {
                                    std::string joined;
                                    for (auto& op : command_queue) {
                                        joined += op + " ";
                                    }
                                    GS.Color.NonStroke = trim_all(joined);
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "gs") {
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "Do") {
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value == "BT") {
                                    is_oper = true;
                                    command_queue.clear();
                                    GS.TextState.Tm = GS.TextState.Tlm = GS.TextState.Trm = Matrix::IdentityMatrix();
                                } else if (value == "ET") {
                                    is_oper = true;
                                    command_queue.clear();
                                } else if (value[0] == 'T') {
                                    switch (value[1]) {
                                        case 'c':
                                        {
                                            GS.TextState.Tc = atof(command_queue[0].c_str());
                                        }
                                        break;
                                        case '*':
                                        {
                                            Matrix Td = Matrix::IdentityMatrix();
                                            Td.x = 0.0;
                                            Td.y = GS.TextState.Tl;
                                            Multiply(Td, GS.TextState.Tlm);
                                            GS.TextState.Tm = GS.TextState.Tlm = Td;
                                        }
                                            break;
                                        case 'd':
                                        {
                                            Matrix Td = Matrix::IdentityMatrix();
                                            Td.x = atof(command_queue[0].c_str());
                                            Td.y = atof(command_queue[1].c_str());
                                            Multiply(Td, GS.TextState.Tlm);
                                            GS.TextState.Tm = GS.TextState.Tlm = Td;
                                        }
                                            break;
                                        case 'D':
                                        {
                                            Matrix Td = Matrix::IdentityMatrix();
                                            Td.x = atof(command_queue[0].c_str());
                                            Td.y = atof(command_queue[1].c_str());
                                            GS.TextState.Tl = -Td.y;
                                            Multiply(Td, GS.TextState.Tlm);
                                            GS.TextState.Tm = GS.TextState.Tlm = Td;
                                        }
                                            break;
                                        case 'f':
                                        {
                                            GS.TextState.Tf = command_queue[0];
                                            auto fnt = GS.TextState.Tf.substr(1);
                                            GS.TextState.Tfont = FontMaps[fnt];
                                            GS.TextState.Tfs = atof(command_queue[1].c_str());
                                            GS.TextState.Tfont->SetSize(GS.TextState.Tfs);
                                        }
                                        break;
                                        case 'r':
                                            GS.TextState.Tmode = atoi(command_queue[0].c_str());
                                            break;
                                        case 'z':
                                            GS.TextState.Th = atof(command_queue[0].c_str());
                                            break;
                                        case 'w':
                                            GS.TextState.Tw = atof(command_queue[0].c_str());
                                            break;
                                        case 'm':
                                            GS.TextState.Tm.a = atof(command_queue[0].c_str());
                                            GS.TextState.Tm.b = atof(command_queue[1].c_str());
                                            GS.TextState.Tm.c = atof(command_queue[2].c_str());
                                            GS.TextState.Tm.d = atof(command_queue[3].c_str());
                                            GS.TextState.Tm.x = atof(command_queue[4].c_str());
                                            GS.TextState.Tm.y = atof(command_queue[5].c_str());
                                            GS.TextState.Tlm = GS.TextState.Tm;
                                            GS.GetUserSpaceMatrix();
                                            break;
                                        case 'j':
                                        {
                                            if (GS.TextState.Tmode == 3) {
                                            } else {
                                                auto copy_ts = GS;
                                                string tj_text = ProcessTextBlock(GS, command_queue[0]);
                                                m_AllTexts.push_back({ copy_ts, tj_text, nPage });
                                            }
                                        }
                                        break;
                                        case 'J':
                                        {
                                            if (GS.TextState.Tmode == 3) {
                                            } else {
                                                for (auto& c : ProcessOperatorTJ(GS, command_queue[0], nPage)) {
                                                    m_AllTexts.push_back({ c.GS, c.text, nPage });
                                                }
                                            }
                                        }
                                        break;
                                    }
                                    is_oper = true;
                                    command_queue.clear();
                                }
                            }
                        }
                    }

                    if (is_oper == false) {
                        if ((first - last_space) > 0) {
                            command_queue.emplace_back(last_space, first);
                        }
                    }

                    if (bEndOfValue) {
                        bEndOfValue = false;
                        if (isspace(*first)) {
                            last_space = first + 1;
                        } else {
                            last_space = first;
                        }
                    } else {
                        last_space = first + 1;
                    }
                }
            }
        }
        nPage++;
    }

    auto test33 = [&](const auto& left, const auto& right) {
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
    };

    sort(m_AllTexts.begin(), m_AllTexts.end(), sort_function);
}

void CPdfDocument::AddObject(CPdfObject* pObject)
{
    m_AllObjects.push_back(pObject);
}

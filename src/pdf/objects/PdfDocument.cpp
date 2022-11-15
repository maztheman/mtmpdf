#include "PdfDocument.h"

#include <cassert>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <array>
#include <unordered_map>

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

#include "pdf/utils/ToUnicode.h"
#include "pdf/utils/tools.h"
#include "pdf/utils/zlib_helper.h"


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

void CPdfDocument::ProcessObject(int object, bool bDoNotProcessIfBroken)
{
    auto sObject = to_string(object);
    if (m_ObjectData.find(sObject) != m_ObjectData.end()) {
        return;
    }
    auto offset = m_ChapterOffset[object];

    if (auto& nonfree = m_ChapterGeneration[object]; nonfree.flag == 'f') {
        return;
    }

    char* data = &m_Data[offset];
    auto line = get_next_line(&data);
    auto checker = ReadObject(line);
    if (checker != object) {
        if (bDoNotProcessIfBroken) {
            return;
        }
    }
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
                m_ChapterGeneration.resize(start_object + count);
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
                m_ChapterGeneration[start_object + i] = SChapterInfo{ (uint32_t)nChapterOffset, (uint32_t)nChapterGeneration, flag[0] };
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
        m_ChapterGeneration.resize(count);
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
            return CPdfString::FromVector(this, dest, len);
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

void CPdfDocument::SetChapterIndex(size_t object, uint32_t offset, uint32_t generation, char flag)
{
    m_ChapterOffset[object] = offset;
    m_ChapterGeneration[object] = { offset, generation, flag };
}

#if 0
static void ProcessParent(SCommandState& state)
{
    const auto& c = state.c;
    const auto& c_last = state.c_last;
    auto& props = state.props;

    //we are in properties, this is 1 thing
    if (c == '<') {
        if (state.c_last == '<') {
            //child properties, is still part of the parent thing
            props.push_back(1);
        }
    } else if (c == '>') {
        if (c_last == '>') {
            if (props.back() == 1) {
                props.pop_back();
            } else {
                fmt::print(stderr, "mis-matched parent/child p:{} c:1\n", props.back());
            }
            //1 thing
            if (state.check_add_sv(state.first + 1)) {
                state.lop = state.first + 1;
            }
        }
    } else if (c == '(') {
        if (props.back() == 2) {//only if direct parent is a ( do we accept \ as an escape character
            if (c_last != '\\') {
                props.push_back(2);
            }
        } else {
            props.push_back(2);
        }
    } else if (c == ')') {
        if (props.back() != 2 || c_last != '\\') {
            if (props.back() == 2) {
                props.pop_back();
            } else {
                fmt::print(stderr, "mis-matched parent/child p:{} c:2\n", props.back());
            }
            if (props.empty() == false && props.back() == 3) {
                //parent array, let it collect me?
                int look = 0;
            } else {
                //1 thing
                if (state.check_add_sv(state.first + 1)) {
                    state.lop = state.first + 1;
                }
            }
        }
    } else if (c == ']') {
        if (props.back() == 3) {
            props.pop_back();
        } else {
            fmt::print(stderr, "mis-matched parent/child p:{} c:3\n", props.back());
        }
        //1 thing
        if (state.check_add_sv(state.first + 1)) {
            state.lop = state.first + 1;
        }
    } else if (c == '/') {
        ProcessTag(state);
    }
}

struct SStackGuard
{
    SCommandState& m_state;

    SStackGuard(SCommandState& s)
        : m_state(s)
    {
        m_state.push();
    }

    ~SStackGuard()
    {
        m_state.pop();
    }
};
#endif

#if 0
/// <summary>
/// Detect if the current value is a command, however if its a command that could be another we need to check to peek forward to see if it is indeed a command but later.
/// </summary>
/// <param name="sFullContents"></param>
/// <returns></returns>
static const SCommandTypeArgs* MaybeCommand(SCommandState& state)
{
    SStackGuard g{ state };
    auto cmd = is_command(state.currentValue());
    bool bCouldBe = false;

    state.next();

    while(state() && ::isalpha(state.c)) {
        
        if (is_command(state.currentValue()) != nullptr) {
            bCouldBe = true;
        }
        
        state.next();
    }

    return cmd;
}
#endif

#if 0
std::vector<SCommand> PreProcessContent(const std::string& sFullContents)
{
    std::vector<SCommand> commands;
    SCommandState state;
    state.intialize(sFullContents);

    for (; state(); state.next()) {
        const char& c = state.c;
        if (state.props.empty() == false) {
            ProcessParent(state);
        } else {
            //command detector
            bool bStar = state.isNext('*'); //if its star lets just ignore this command looking thing
            if (bStar == false) {
                if (auto cmd = MaybeCommand(state); cmd) {
                    
                    if (cmd->argCount >= 0) {
                        if (static_cast<int>(state.retval.size()) > cmd->argCount) {
                            int look = 0;
                            //this could be bad or could be nothing
                        }
                    }
                    
                    commands.emplace_back(SCommand{ cmd->command, std::move(state.retval) });

                    state.lop = state.first + (isspace(c) ? 1 : 0);
                } else {
                    if (isalnum(c)) {
                        if (isalpha(state.c_last) && isdigit(c)) {
                            //special case..
                            //check to see if cm0 could possibly be a command then a 0
                            if (auto cmd = is_command(state.svContents.substr(state.lop - state.beg, state.first - state.lop)); cmd) {
                                state.check_add_sv(state.first);
                                state.lop = state.first;
                            }
                        }
                    } else if (isspace(c)) {
                        //"end of line"
                        if (isspace(state.c_last)) {
                            //dont add a space if the last char is a space
                            state.lop = state.first + 1;
                        } else {
                            state.check_add_sv(state.first);
                            state.lop = state.first + 1;
                        }
                    } else if (c == '<') {
                        if (state.c_last != '<') {
                            if (state.check_add_sv(state.first)) {
                                state.lop = state.first;
                            }
                        } else {
                            state.props.push_back(1);
                        }
                    } else if (c == '(') { //array 
                        if (state.check_add_sv(state.first)) {
                            state.lop = state.first;
                        }
                        state.props.push_back(2);
                    } else if (c == '[') {
                        if (state.check_add_sv(state.first)) {
                            state.lop = state.first;
                        }
                        state.props.push_back(3);
                    } else if (c == '/') {
                        //process TAG, a tag can have letters that would otherwise be classified as commands
                        ProcessTag(state);
                    }
                }
            }
        }
    }

   
    /*std::vector<std::string_view> command_queue;
    for (auto& sv : state.retval) {
        if (auto cmd = is_command(sv); cmd) {
            if (cmd->argCount >= 0) {
                if (static_cast<int>(command_queue.size()) > cmd->argCount) {
                    int look = 0;
                    //this could be bad or could be nothing
                }
            }

            commands.emplace_back(SCommand{ cmd->command, std::move(command_queue) });
        } else {
            command_queue.push_back(sv);
        }
    }

    return state.retval;*/

    return commands;
}


#endif


std::vector<std::tuple<CPdfPage*, std::string>> CPdfDocument::GetPageContents()
{
    CPdfCatalog* pCatalog = static_cast<CPdfCatalog*>(GetObjectData("root"));
    pCatalog->Process();

    std::vector<std::tuple<CPdfPage*, std::string>> pageContents;
    pageContents.reserve(pCatalog->m_pPages->m_Data.size());

    for (auto pPage : pCatalog->m_pPages->m_Data) {
        pPage->Process();
        auto media_box = pPage->m_pDictionary->GetProperty("MediaBox");
        if (media_box) {
            m_MediaBoxes.push_back(media_box->c_str());
        } else {
            m_MediaBoxes.push_back("");
        }

        size_t maxSize = 0;
        for (auto& content : pPage->m_Contents) {
            maxSize += content.size();
        }

        std::string sFullContents;
        sFullContents.reserve(maxSize);

        auto it = pPage->m_ContentReference.begin();

        for (auto& content : pPage->m_Contents) {
            sFullContents += content;
            ++it;
        }

        pageContents.emplace_back(std::make_tuple(pPage, std::move(sFullContents)));
    }

    return pageContents;
}


#if 0

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

bool bEndOfValue = false;

//pdf works like a queue, where things are pushed into the queue then executed
char* first = &sFullContents[0];
char* last = &sFullContents[sFullContents.size() - 1];
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
                case 0x0A:
                    //k cmyk colour non-stroke
                    GS.ColorSpace.NonStroke = "/DeviceCMYK";
                    {
                        std::string joined;
                        for (auto& op : command_queue) {
                            joined += op + " ";
                        }
                        GS.Color.NonStroke = trim_all(joined);
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
                case 0x2A:
                    //K cmyk colour non-stroke
                    GS.ColorSpace.Stroke = "/DeviceCMYK";
                    {
                        std::string joined;
                        for (auto& op : command_queue) {
                            joined += op + " ";
                        }
                        GS.Color.Stroke = trim_all(joined);
                    }
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

                            if (command_queue[4] == "40.2621" && command_queue[5] == "374.3417") {
                                double look = GS.TextState.Tm.x;
                            }


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


#endif        

void CPdfDocument::AddObject(CPdfObject* pObject)
{
    m_AllObjects.push_back(pObject);
}

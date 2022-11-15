#include "PdfDictionary.h"
#include "PdfString.h"

#include "pdf/utils/tools.h"

#include <string_view>

using namespace std;

static inline vector<char> read_array(char*& first, char* last)
{
    int level = 0;
    vector<char> retval;
    for (; first != last; ++first) {
        if (*first == '[') {
            level++;
            retval.push_back(*first);
        } else if (*first == ']') {
            if (level > 0) {
                level--;
                retval.push_back(*first);
            } else {
                ++first;
                break;
            }
        } else {
            retval.push_back(*first);
        }
    }

    return retval;
}

static inline vector<char> read_brackets(char*& first, char* last)
{
    vector<char> retval;
    for (; first != last; ++first) {
        if (*first == '(') {
            auto arr = read_array(++first, last);
            retval.insert(retval.end(), arr.begin(), arr.end());
        } else if (*first == ')') {
            ++first;
            break;
        } else {
            retval.push_back(*first);
        }
    }

    return retval;
}

using svi = std::string_view::const_iterator;


bool is_property_start(const char ch)
{
    return ch == '<';
};

bool is_tag_start(const char ch)
{
    return ch == '/';
}

std::string_view fast_forward_to_property(std::string_view sv)
{
    auto first = sv.begin();
    char ch_last = 0;
    for (; first != sv.end(); ++first) {
        const char ch = *first;
        if (is_property_start(ch) && is_property_start(ch_last)) {
            return sv.substr(first + 1 - sv.begin());
        }
        ch_last = ch;
    }
    return sv.substr(first - sv.begin());
}



//Tag then space means there is a value
std::string_view ProcessTag()
{
    return ""sv;
}

std::tuple<std::string_view, std::map<std::string, CPdfObject*>> read_directory(CPdfDocument* pDocument, std::string_view sv)
{
    std::map<std::string, CPdfObject*> prop;

    sv = fast_forward_to_property(sv);

    svi end = sv.end();
    svi beg = sv.begin();
    auto first = sv.begin();

    const auto is_end = [&end](svi it) -> bool {
        return it != end;
    };

    char ch_last = 0;

    for (; is_end(first); ++first) {
        const char ch = *first;
        if (is_tag_start(ch)) {

        }

        ch_last = ch;
    }



    return std::make_tuple(sv.substr(first - beg), std::move(prop));
}



static inline map<string, CPdfObject*> read_raw_dictionary(CPdfDocument* pDocument, char*& first, char* last)
{
    map<string, CPdfObject*> retval;
    for (; first != last; first++) {
        if (*first == '<' && *(first + 1) == '<') {
            first += 2;
            break;
        } else if (*first == '[') {
            auto vv = read_array(++first, last);
            --first;
        }
    }
    vector<char> p;
    bool b = false;
    for (; first != last; ++first) {
        if (*first == '<' && *(first + 1) == '<') {
            auto dict = CPdfDictionary::FromRawString(pDocument, first, last);
            retval.insert(make_pair(string(p.begin(), p.end()), dict));
            --first;
            b = false;
        } else if (*first == '>' && *(first + 1) == '>') {
            first += 2;
            break;
        } else if (*first == '/') {
            first++;
            if (b == false) {
                bool embedded_value = false, in_value = false;
                vector<char> vv;
                p.clear();
                for (; first != last; ++first) {
                    if (is_regular_tag(*first)) {
                        if (*first == '(') {
                            embedded_value = true;
                            in_value = true;
                        } else if (*first == ')') {
                            in_value = false;
                        } else if (in_value) {
                            vv.push_back(*first);
                        } else {
                            p.push_back(*first);
                        }
                    } else if (in_value) {
                        vv.push_back(*first);
                    } else {
                        --first;
                        break;
                    }
                }
                b = true;

                if (embedded_value) {
                    retval.insert(make_pair(string(p.begin(), p.end()), CPdfString::FromVector(pDocument, vv)));
                    b = false;
                }

            } else {
                vector<char> vv;
                for (; first != last; ++first) {
                    if (*first == '>' && *(first + 1) == '>') {
                        --first;
                        break;
                    } else if (*first == '/') {
                        --first;
                        break;
                    } else if (*first == '\n') {
                        continue;
                    }
                    vv.push_back(*first);
                }

                retval.insert(make_pair(string(p.begin(), p.end()), CPdfString::FromVector(pDocument, vv)));
                b = false;
            }
        } else if (isspace(*first)) {
            ++first;
            vector<char> vv;
            for (; first != last; ++first) {
                if (*first == '<' && *(first + 1) == '<') {
                    auto dict = CPdfDictionary::FromRawString(pDocument, first, last);
                    retval.insert(make_pair(string(p.begin(), p.end()), dict));
                    --first;
                    b = false;
                    break;
                } else if (*first == '>' && *(first + 1) == '>') {
                    --first;
                    break;
                } else if (*first == '/') {
                    --first;
                    break;
                } else if (*first == '\n') {
                    continue;
                } else if (*first == '[') {
                    vv = read_array(++first, last);
                    --first;
                    break;
                } else if (*first == '(') {
                    vv = read_brackets(++first, last);
                    --first;
                    break;
                }
                vv.push_back(*first);
            }
            if (vv.empty()) {
            } else {
                b = false;
                retval.insert(make_pair(trim_all(string(p.begin(), p.end())), CPdfString::FromVector(pDocument, vv)));
            }
        } else if (*first == '[') {
            b = false;
            auto vv = read_array(++first, last);
            retval.insert(make_pair(string(p.begin(), p.end()), CPdfString::FromVector(pDocument, vv)));
            --first;
        } else if (*first == '(') {
            b = false;
            auto vv = read_brackets(++first, last);
            retval.insert(make_pair(string(p.begin(), p.end()), CPdfString::FromVector(pDocument, vv)));
            --first;
        }
    }
    return retval;
}

CPdfDictionary::CPdfDictionary(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}

CPdfDictionary::~CPdfDictionary()
{
}

CPdfDictionary* CPdfDictionary::FromRawString(CPdfDocument* pDocument, char*& first, char* last)
{
    CPdfDictionary* pObj = new CPdfDictionary(pDocument);
    pObj->m_Data = read_raw_dictionary(pDocument, first, last);
    return pObj;
}

CPdfDictionary* CPdfDictionary::FromString(CPdfDocument* pDocument, string sValue)
{
    char* first = &sValue[0];
    char* last = &sValue[sValue.size() - 1];
    return FromRawString(pDocument, first, last);
}

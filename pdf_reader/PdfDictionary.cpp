#include "stdafx.h"
#include "PdfDictionary.h"
#include "PdfString.h"

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
                p.clear();
                for (; first != last; ++first) {
                    if (is_regular(*first)) {
                        p.push_back(*first);
                    } else {
                        --first;
                        break;
                    }
                }
                b = true;
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

#include "stdafx.h"
#include "tools.h"
#include "PdfObject.h"
#include "PdfTypes.h"

using namespace std;

std::vector<std::string> split_on_space(string value) 
{
    vector<string> retval;
    auto res = count(value.begin(), value.end(), ' ');
    retval.reserve(res + 1);
    string token;
    for (const auto c : value) {
        if (!isspace(c)) {
            token += c;
        } else {
            if (token.empty() == false) {
                retval.emplace_back(std::move(token));
            }
        }
    }
    if (token.empty() == false) {
        retval.emplace_back(std::move(token));
    }
    return retval;
}

std::vector<std::string> split_bt_parts(const std::string& value)
{
    vector<string> retval;
    auto res = count(value.begin(), value.end(), ' ');
    retval.reserve(res + 1);
    string token;
    bool bOpenBracet = false;
    bool bIgnoreNextChar = false;
    for (const auto c : value) {
        if (c == '\\') {
            bIgnoreNextChar = true;
            continue;
        } else if (bIgnoreNextChar) {
            token += c;
        } else if (c == '(') {
            bOpenBracet = true;
        } else if (c == ')') {
            bOpenBracet = false;
            if (token.empty() == false) {
                retval.emplace_back(std::move(token));
            }
        } else if (bOpenBracet) {
            token += c;
        } else if (!isspace(c)) {
            token += c;
        } else {
            if (token.empty() == false) {
                retval.emplace_back(std::move(token));
            }
        }
        bIgnoreNextChar = false;
    }
    if (token.empty() == false) {
        retval.emplace_back(std::move(token));
    }
    return retval;
}


std::vector<std::string> split_bt(const std::string& value)
{
    vector<string> texts;
    size_t offset = 0;
    for (;;) {
        auto bt = value.find("BT", offset);
        if (bt == string::npos) {
            break;
        }
        auto et = value.find("ET", bt);
        texts.emplace_back(value.substr(bt + 2, et - (bt + 2)));
        offset = et + 2;
    }
    return texts;
}

string trim_all(string value)
{
    if (value.empty()) {
        return value;
    }
    while (value.empty() == false && isspace((unsigned char)value[0])) {
        value.erase(0, 1);
    }
    while (value.empty() == false && isspace((unsigned char)value.back())) {
        value.erase(value.size() - 1, 1);
    }
    return value;
}

int ObjectFromReference(CPdfObject* pObj)
{
    if (pObj->GetType() != pdf_type::string) {
        return -1;
    }
    auto values = split_on_space(trim_all(pObj->c_str()));
    if (values.size() != 3 || values[2] != "R") {
        return -1;
    }
    return atoi(values[0].c_str());
}

int ReadObject(string value)
{
    auto values = split_on_space(trim_all(value));
    if (values.size() != 3 || values[2] != "obj") {
        return -1;
    }
    return atoi(values[0].c_str());
}

string preview_next_line(char* data)
{
    data = skip_space(data);
    auto nl = find_space(data);
    string retval(data, nl);
    return retval;
}

char* skip_space(char* data)
{
    for (; *data == '\r' || *data == '\n'; ++data);
    return data;
}

char* find_space(char* data)
{
    for (; *data != '\r' && *data != '\n'; ++data);
    return data;
}

string get_next_line(char** data)
{
    *data = skip_space(*data);
    auto nl = find_space(*data);
    string retval(*data, nl);
    *data = nl;
    return retval;
}

uint32_t read_dynamic_size(char* data, int size)
{
    uint32_t retval = 0;
    switch (size) {
        case 0:
            retval = -1;
            break;
        case 1:
            retval = *reinterpret_cast<uint8_t*>(data);
            break;
        case 2:
            retval = static_cast<uint8_t>(data[0]) << 8 | static_cast<uint8_t>(data[1]);
            break;
        case 3:
            retval = static_cast<uint8_t>(data[0]) << 16 | static_cast<uint8_t>(data[1]) << 8 | static_cast<uint8_t>(data[2]);
            break;
        case 4:
            retval = static_cast<uint8_t>(data[0]) << 24 | static_cast<uint8_t>(data[1]) << 16 | static_cast<uint8_t>(data[2]) << 8 | static_cast<uint8_t>(data[3]);
            break;
        default:
            retval = 0;
            break;
    }

    return retval;
}

bool is_number(const string& text)
{
    for (auto& c : text) {
        if (isdigit(c) || c == '.' || c == '(' || c == ')' || c == ',') {
        } else {
            return false;
        }
    }
    return true;
}

void Multiply(Matrix& current, const Matrix& new_value)
{
    double _11, _12, _21, _22, _31, _32;

    _11 = current.a * new_value.a + current.b * new_value.c;
    _12 = current.a * new_value.b + current.b * new_value.d;
    _21 = current.c * new_value.a + current.d * new_value.c;
    _22 = current.c * new_value.b + current.d * new_value.d;
    _31 = current.x * new_value.a + current.y * new_value.c + 1.0 * new_value.x;
    _32 = current.x * new_value.b + current.y * new_value.d + 1.0 * new_value.y;


    /*_11 = new_value.a * current.a + new_value.b * current.c + 0.0 * current.x;
    _12 = new_value.a * current.b + new_value.b * current.d + 0.0 * current.y;
    _21 = new_value.c * current.a + new_value.d * current.c + 0.0 * current.x;
    _22 = new_value.c * current.b + new_value.d * current.d + 0.0 * current.y;
    _31 = new_value.x * current.a + new_value.y * current.c + 1.0 * current.x;
    _32 = new_value.x * current.b + new_value.y * current.d + 1.0 * current.y;*/

    current.a = _11;
    current.b = _12;
    current.c = _21;
    current.d = _22;
    current.x = _31;
    current.y = _32;
}

static inline uint16_t HexCharToWord(char t)
{
    uint16_t retval = 0;
    if (t >= '0' && t <= '9') {
        retval |= (t - '0');
    } else if (t >= 'a' && t <= 'f') {
        retval |= 10 + (t - 'a');
    } else if (t >= 'A' && t <= 'F') {
        retval |= 10 + (t - 'A');
    }
    return retval;
}

std::vector<uint16_t> HexTo16BitArray(string hex_string)
{
    std::vector<uint16_t> retval;
    int extra = (hex_string.size() & 3);

    if (extra != 0) {
        for (int h = 0; h < extra; h++) {
            hex_string.append("0");
        }
    }

    for (size_t n = 0; n < hex_string.size(); n += 4) {
        uint16_t cc = 0;
        cc |= HexCharToWord(hex_string[n + 0]) << 12;
        cc |= HexCharToWord(hex_string[n + 1]) << 8;
        cc |= HexCharToWord(hex_string[n + 2]) << 4;
        cc |= HexCharToWord(hex_string[n + 3]);
        retval.push_back(cc);
    }

    return retval;
}

bool is_white_space(char c)
{
    bool rc = false;
    switch (c) {
        case 0:
        case 9:
        case 10:
        case 12:
        case 13:
        case 32:
            rc = true;
            break;
        default:
            rc = false;
            break;
    }
    return rc;
}

bool is_delimeter(char c)
{
    bool rc = false;
    switch (c)         {
        case '(':
        case ')':
        case '<':
        case '>':
        case '[':
        case ']':
        case '{':
        case '}':
        case '/':
        case '%':
            rc = true;
            break;
        default:
            rc = false;
            break;
    }
    return rc;
}

bool is_regular(char c)
{
    return is_white_space(c) == false && is_delimeter(c) == false;
}


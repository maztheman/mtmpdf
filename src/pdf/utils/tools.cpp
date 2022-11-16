#include "tools.h"

#include <ranges>
#include <algorithm>
#include <string>
#include <cctype>

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

std::vector<std::string> split_on_command(std::string value)
{
    std::vector<string> retval;
    auto res = count(value.begin(), value.end(), ' ');
    retval.reserve(res + 1);
    string token;
    char chLastChar = '\0';
    for (const auto c : value) {
        bool bPushToken = false;
        if (isspace(c)) {
            if (token.empty() == false) {
                retval.emplace_back(std::move(token));
            }
        } else if (c == '<' || c == '>') {
            if (chLastChar != c) {
                if (token.empty() == false) {
                    retval.emplace_back(std::move(token));
                }
            }
            token += c;
        } else if (c == '(') {
            if (token.empty() == false) {
                retval.emplace_back(std::move(token));
            }
            token += c;
        } else if (c == '\\') {
            if (token.empty() == false) {
                retval.emplace_back(std::move(token));
            }
            token += c;
        } else if (c == '/') {
            if (token.empty() == false) {
                retval.emplace_back(std::move(token));
            }
            token += c;
        } else {
            token += c;
        }
        chLastChar = c;
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



int ReadObject(string value)
{
    //split on space, command or special character so " ", "<<" "(" "<"
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
    for (; *data != '\r' && *data != '\n' && *data != '<'; ++data);
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
    switch (c) {
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

bool is_delimeter_tag(char c)
{
    bool rc = false;
    switch (c) {
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

bool is_regular_tag(char c)
{
    return is_white_space(c) == false && is_delimeter_tag(c) == false;
}



#if defined(_WIN32)

std::string str_from_wstr(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string str_from_wstr(const std::vector<wchar_t>& wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}
#else

std::string str_from_wstr(const std::wstring& wstr)
{
    return std::string{wstr.begin(), wstr.end()};
}


std::string str_from_wstr(const std::vector<wchar_t>& wstr)
{
    return std::string{wstr.begin(), wstr.end()};
}
#endif

char mylower(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        c = c | 0x20;
    }
    return c;
}

bool iequals(std::string_view lhs, std::string_view rhs)
{
//    constexpr auto to_lower = std::views::transform(std::tolower);
    return std::ranges::equal(
        lhs | std::views::transform(mylower),
        rhs | std::views::transform(mylower)
    );
}

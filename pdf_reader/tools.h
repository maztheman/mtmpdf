#pragma once

class CPdfObject;
struct Matrix;

int ObjectFromReference(CPdfObject* pObj);
int ReadObject(std::string value);
std::string trim_all(std::string value);
std::vector<std::string> split_on_space(std::string value);
std::vector<std::string> split_bt(const std::string& value);
std::vector<std::string> split_bt_parts(const std::string& value);

//Special Data Processing
std::string preview_next_line(char* data);
char* skip_space(char* data);
char* find_space(char* data);
std::string get_next_line(char** data);
uint32_t read_dynamic_size(char* data, int size);
bool is_number(const std::string& text);


bool is_white_space(char c);
bool is_delimeter(char c);
bool is_regular(char c);

//Current = New_Value * Current;
void Multiply(Matrix& current, const Matrix& new_value);

std::vector<uint16_t> HexTo16BitArray(std::string hex_string);
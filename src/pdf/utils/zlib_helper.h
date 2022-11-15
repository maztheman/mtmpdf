#pragma once
#include <vector>

int InflateData(std::vector<char>& buffer, std::vector<char>& dest);
int DeflateData(std::vector<char>& buffer, std::vector<char>& dest);
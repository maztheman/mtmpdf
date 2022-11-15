#pragma once

class CCustomTextState;

class CCSVExporter
{
public:
    CCSVExporter();
    ~CCSVExporter();

    static bool ToFile(const std::string& Filename, const std::vector<std::string>& state);
    static bool ToFile(const std::string& Filename, const CCustomTextState& state);
};


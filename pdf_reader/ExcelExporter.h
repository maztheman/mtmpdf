#pragma once

class CCustomTextState;
class CCustomState2;

class CExcelExporter
{
public:
    CExcelExporter();
    ~CExcelExporter();

    static bool ToFile(const std::string& Filename, const CCustomTextState& state);
    static bool ToFile(const std::string& Filename, const CCustomState2& state);
};


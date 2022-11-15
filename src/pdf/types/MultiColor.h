#pragma once

struct SMultiColor
{
    std::string         Stroke;
    std::string         NonStroke;


    static SMultiColor FromDefault(const std::string& Default)
    {
        SMultiColor retval;
        retval.Stroke = Default;
        retval.NonStroke = Default;
        return retval;
    }
};

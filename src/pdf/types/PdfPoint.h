#pragma once

struct PdfPoint
{
    PdfPoint(double in_x, double in_y)
        : x(in_x), y(in_y)
    {

    }
    double x, y;
};

using subpath_t = std::vector<PdfPoint>;
using path_t = std::vector<subpath_t>;

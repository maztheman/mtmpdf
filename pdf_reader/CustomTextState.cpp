#include "stdafx.h"
#include "CustomTextState.h"

#include <deque>

using std::string;
using std::deque;
using std::vector;

CCustomTextState::CCustomTextState()
{
}


CCustomTextState::~CCustomTextState()
{
}

std::vector<const VerticalLine*> CCustomTextState::CollectLinesPerPage(size_t nPage)
{
    vector<const VerticalLine*> retval;
    for (const auto& line : vertical_lines) {
        if (line.nPage == nPage) {
            retval.push_back(&line);
        }
    }
    return retval;
}

const VerticalLine* CCustomTextState::FindClosestToTopLeft(double x, double y, size_t nPage)
{
    double diff_y = 9999;
    double diff_x = 9999;
    size_t idx = ~0;
    double dBestY = 0.0;

    //get candidates in order of diff
    deque<size_t> size;

    auto lines = CollectLinesPerPage(nPage);

    //find Closest to top
    for (size_t n = 0; n < lines.size(); n++) {
        auto& line = *lines[n];
        auto dy = abs(line.y - y);
        if (line.x <= x) {
            //its to the left
            if (line.y > y) {
                if (dy <= diff_y) {
                    diff_y = dy;
                    dBestY = line.y;
                    size.push_back(n);
                }
            }
        }
    }

    while (size.empty() == false) {
        size_t test = size.back();
        size.pop_back();
        auto& line = *lines[test];
        if (line.y == dBestY) {
            auto dx = abs(line.x - x);
            if (dx < diff_x) {
                diff_x = dx;
                idx = test;
            }
        }
    }

    if (idx == ~0) {
        return nullptr;
    }

    return lines[idx];
}

const VerticalLine* CCustomTextState::FindClosestToTopRight(double x, double y, size_t nPage)
{
    double diff_y = 9999;
    double diff_x = 9999;
    size_t idx = ~0;
    double dBestY = 0.0;

    deque<size_t> size;
    auto lines = CollectLinesPerPage(nPage);

    //find Closest to top
    for (size_t n = 0; n < lines.size(); n++) {
        auto& line = *lines[n];
        auto dy = abs(line.y - y);
        if (line.x > x) {
            //its to the left
            if (line.y > y) {
                if (dy <= diff_y) {
                    diff_y = dy;
                    dBestY = line.y;
                    size.push_back(n);
                }
            }
        }
    }

    while (size.empty() == false) {
        size_t test = size.back();
        size.pop_back();
        auto& line = *lines[test];
        if (line.y == dBestY) {
            auto dx = abs(line.x - x);
            if (dx < diff_x) {
                diff_x = dx;
                idx = test;
            }
        }
    }

    if (idx == ~0) {
        return nullptr;
    }

    return lines[idx];
}
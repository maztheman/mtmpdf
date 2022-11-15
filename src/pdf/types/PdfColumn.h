#pragma once

#include "PdfRect.h"


//this is a business logic kinda of object
struct PdfColumn
{
    PdfRect rect;
    PdfRect maybe_rect;
    bool has_maybe_rect;
    bool header_is_centered;
    std::string name;
};



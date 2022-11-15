#pragma once 

#include <pdf/types/GraphicsState.h>

namespace processor::pdf::types {
    struct TextAtLocation
    {
        DIGraphicsState GS;
        std::string text;
        size_t nPage;
    };
}

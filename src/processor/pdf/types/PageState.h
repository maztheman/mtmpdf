#pragma once

#include "pdf/types/GraphicsState.h"
#include "pdf/types/PathState.h"

#include <processor/pdf/types/TextAtLocation.h>
#include <processor/types/Line.h>

class CPdfPage;

namespace processor::pdf
{
    struct SPageState
    {
        DIGraphicsState GS;
        PathState       PS;
        std::deque<DIGraphicsState> GraphicsStateQueue;
        std::vector<processor::types::VerticalLine> AllVerticalLines;
        std::vector<processor::types::HorizontalLine> AllHorizontalLines;
        std::vector<types::TextAtLocation> AllTexts;
        CPdfPage* page;
        size_t nPage;
        std::map<std::string, CPdfFont*> FontMaps;

        SPageState(CPdfPage* page, size_t pageNum);
    };
}
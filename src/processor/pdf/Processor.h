#pragma once

#include <parser/pdf/Command.h>

#include <processor/pdf/types/TextAtLocation.h>
#include <processor/types/Line.h>

class CPdfDocument;

namespace processor::pdf {
	struct SProcessedText
	{
		std::vector<processor::types::VerticalLine> AllVerticalLines;
		std::vector<processor::types::HorizontalLine> AllHorizontalLines;
		std::vector<types::TextAtLocation> AllTexts;
	};

	SProcessedText ProcessText(CPdfDocument* pDocument);
}
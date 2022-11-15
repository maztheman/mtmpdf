#include "DumpText.h"

#include <pdf/objects/PdfDocument.h>
#include <pdf/objects/PdfObject.h>
#include <processor/pdf/Processor.h>

#include <filesystem>

namespace fs = std::filesystem;

void DumpUnzippedPdf(CPdfDocument* pDocument)
{
	fs::path pp = pDocument->GetFilename();

	std::string sJustText;

	auto count = pDocument->GetObjectCount();
	for (size_t n = 1; n < count; n++) {
		pDocument->ProcessObject((int)n, true);
	}

	for (const auto& pObj : pDocument->GetAllObjects()) {
		if (pObj) {
			sJustText += pObj->c_str();
			sJustText += "\r\n";
		}
	}

	auto jt = fmt::format("{}_{}.txt", pp.generic_string(), "uz");
	std::fstream f(jt, std::ios::out | std::ios::ate);
	f.write(sJustText.data(), sJustText.size());

}

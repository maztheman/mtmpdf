#include "DumpText.h"

#include <pdf/objects/PdfDocument.h>
#include <pdf/objects/PdfObject.h>
#include <processor/pdf/Processor.h>

#include <filesystem>

namespace fs = std::filesystem;

void DumpUnzippedPdf(CPdfDocument* pDocument)
{
	fs::path outPath = pDocument->GetFilename();

	std::string sJustText;

	auto count = pDocument->GetObjectCount();
	for (size_t n = 1; n < count; n++) 
	{
		pDocument->ProcessObject(static_cast<int>(n), true);
	}

	for (const auto& pObj : pDocument->GetAllObjects()) {
		if (pObj) {
			sJustText += pObj->c_str();
			sJustText += "\r\n";
		}
	}

	fs::path newFilename =
		outPath.filename().append("_uz");

	outPath
		.replace_filename(newFilename)
		.replace_extension(".txt");

	std::fstream f(outPath, std::ios::out | std::ios::ate);
	f.write(sJustText.data(), static_cast<std::streamsize>(sJustText.size()));

}

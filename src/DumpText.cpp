#include "DumpText.h"

#include <pdf/objects/PdfDocument.h>
#include <processor/pdf/Processor.h>

#include <filesystem>

namespace fs = std::filesystem;

void DumpText(CPdfDocument* pDocument)
{
    fs::path outDir = pDocument->GetFilename();

    auto processedText = processor::pdf::ProcessText(pDocument);

    std::sort(processedText.AllTexts.begin(), processedText.AllTexts.end(), [&](const auto& left, const auto& right) {
        size_t nLeftPage = left.nPage;
        size_t nRightPage = right.nPage;

        if (nLeftPage < nRightPage) {
            return true;
        } else if (nLeftPage > nRightPage) {
            return false;
        }

        Matrix trm_left = left.GS.GetUserSpaceMatrix();
        Matrix trm_right = right.GS.GetUserSpaceMatrix();

        double dLeftY = trm_left.y;
        double dRightY = trm_right.y;

        double diff_y = abs(dLeftY - dRightY);

        bool is_same = diff_y < 4.0;

        double dLeftX = trm_left.x;
        double dRightX = trm_right.x;

        if (is_same) {
            return dLeftX < dRightX;
        }

        if (dLeftY > dRightY) {
            return false;
        } else if (dLeftY < dRightY) {
            return true;
        }
        return false;
        });

    auto& all_texts = processedText.AllTexts;

    std::string sJustText;

    for (auto& txt : all_texts) {
        sJustText += fmt::format("{}", txt.text);
    }

    fs::path newFilename = 
        outDir.filename().append(fmt::format("_jt"));

    outDir
        .replace_filename(newFilename)
        .replace_extension(".txt");

    std::fstream f(outDir, std::ios::out | std::ios::ate);
    f.write(sJustText.data(), static_cast<std::streamsize>(sJustText.size()));
}
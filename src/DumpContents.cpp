#include "DumpContents.h"

#include <pdf/objects/PdfDocument.h>
#include <pdf/objects/PdfPages.h>
#include <pdf/objects/PdfPage.h>
#include <pdf/objects/PdfCatalog.h>
#include <pdf/objects/PdfString.h>
#include <pdf/utils/zlib_helper.h>
#include <pdf/utils/tools.h>

#include <filesystem>

namespace fs = std::filesystem;

void DumpContents(CPdfDocument* pDocument)
{
    fs::path outDir = pDocument->GetFilename();
    auto pages = pDocument->GetPageContents();
    
    for (size_t pageNum = 0; auto&& [pPage, sContents] : pages) 
    {
        fs::path newFilename = outDir.filename().append(fmt::format("_p{}", pageNum++));
        outDir
            .replace_filename(newFilename)
            .replace_extension(".txt");
        std::fstream fi(outDir, std::ios::out | std::ios::ate | std::ios::binary);
        fi.write(sContents.data(), static_cast<std::streamsize>(sContents.size()));
    }

    CPdfCatalog* pCatalog = static_cast<CPdfCatalog*>(pDocument->GetObjectData("root"));
    for (auto pPage : pCatalog->m_pPages->m_Data) {
        auto it = pPage->m_ContentReference.begin();
        for (auto& content : pPage->m_Contents) {
            {
                auto fileName = fmt::format("{}_{}.txt", outDir.generic_string(), *it);
                std::fstream fi(fileName, std::ios::out | std::ios::ate | std::ios::binary);
                fi.write(content.data(), static_cast<std::streamsize>(content.size()));
            }
            
            for (auto fnd = content.find("CAODC"); fnd != std::string::npos; fnd = content.find("CAODC")) {
                fmt::print("found in {}\n", *it);
                content[fnd + 3] = 'E';
            }

            std::vector<char> orig(content.begin(), content.end()), dest, andback;

            orig.insert(orig.end(), {'\r', '\n'});

            DeflateData(orig, dest);
            {
                if (auto pStr = dynamic_cast<CPdfString*>(pDocument->GetObjectData(*it)); pStr) {
                    if (dest.size() < pStr->m_OriginalSize) {
                        dest.resize(pStr->m_OriginalSize, ' ');
                    }
                }
                fs::path newFilename = outDir.filename().append(fmt::format("_{}_z", *it));
                outDir
                    .replace_filename(newFilename)
                    .replace_extension(".txt");                
                std::fstream fi(outDir, std::ios::out | std::ios::ate | std::ios::binary);
                fi.write(dest.data(), static_cast<std::streamsize>(dest.size()));
            }

            InflateData(dest, andback);

            ++it;
        }
    }

}

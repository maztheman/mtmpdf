#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
#include <thread>

#include "pdf/objects/PdfDocument.h"
#include "PdfToExcel.h"
#include "ScrapePart1.h"
#include "ExcelExporter1.h"
#include "pdf/utils/tools.h"

#include "DumpText.h"
#include "DumpContents.h"
#include "DumpUnzippedPdf.h"

#include <pdf/objects/PdfDictionary.h>

#include <docopt/docopt.h>

using namespace std;

class file_data
{
    FILE* m_pData;
public:
    file_data(string fn)
        : m_pData(nullptr)
    {
        m_pData = fopen(fn.c_str(), "rb");
    }

    ~file_data()
    {
        if (m_pData) {
            fclose(m_pData);
            m_pData = nullptr;
        }
    }

    void ReadAllData()
    {
        size_t size;
        fseek(m_pData, 0, SEEK_END);
        size = ftell(m_pData);
        rewind(m_pData);
        m_Data.resize(size, 0);
        fread(&m_Data[0], 1, size, m_pData);
    }

    vector<char> m_Data;
};

namespace fs = std::filesystem;


static constexpr auto USAGE =
R"(Pdf Reader.

    Usage: pdf_reader [-d PATH] [-ecr]
           pdf_reader (-h | --help)
           pdf_reader --version

 Options:
          -h --help            Show this screen.
          --version            Show version.
          -d PATH, --dir PATH  Directory read, will read all pdfs in this directory [default: .].
          -c --export-content  Exports the PS code
          -r --export-rawtext  Exports the text sorted by location (top to bottom, left to right)
          -e                   Test
)";

int main(int argc, char* argv[])
{
    std::string sTest = "<</DecodeParms<</Columns 5/Predictor 12>>/Filter/FlateDecode/ID[<918C126C546ABAB269566430BEF66983><FE6A95E960510547963E2EC18D5715CA>]/Index[105501 953]/Info 105500 0 R/Length 156/Prev 3176292/Root 105502 0 R/Size 106454/Type/XRef/W[1 3 1]>>";

    CPdfDocument doc("dummy", std::vector<char>{});

    auto cont = read_directory(&doc, sTest);


    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        { std::next(argv), std::next(argv, argc) },
        true,// show help if requested
        "PDF Reader 0.1");// version string

    std::string base_dir;

    {
        string tmp = args["--dir"].asString();
        if (tmp.back() == '\\' || tmp.back() == '\"') {
            tmp.erase(tmp.size() - 1, 1);
        }
        fs::path file(tmp);
        base_dir = file.string();
        if (base_dir.empty()) {
            fmt::print(stderr, "Path not found {}\n", argv[1]);
            fs::path file_backup(argv[0]);
            base_dir = file_backup.parent_path().string();
        }
    }

    for (auto& p : fs::directory_iterator(base_dir)) {
        fs::path file(p);
        if (file.extension() == L".pdf" || file.extension() == L".PDF") {
            string sName = file.string();
            file_data fd(sName);
            fd.ReadAllData();
            auto document = std::make_unique<CPdfDocument>(sName, std::move(fd.m_Data));
            document->Initialize();

            if (args["--export-content"].asBool()) {
                fmt::print(stderr, "Exporting raw PS Code for PDF '{}'\n", sName);
                DumpContents(document.get());
            }

            if (args["--export-rawtext"].asBool()) {
                fmt::print(stderr, "Exporting raw Text for PDF '{}'\n", sName);
                DumpText(document.get());
            }

            if (args["-e"].asBool()) {
                fmt::print(stderr, "Exporting unzipped pdf '{}'\n", sName);
                DumpUnzippedPdf(document.get());
            }
        }
    }

    printf("End of conversion\n");
    return 0;
}

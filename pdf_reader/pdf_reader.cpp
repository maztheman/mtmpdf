// pdf_reader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

#include <vector>
#include <fstream>
#include <string>
#include <thread>

#include "PdfDocument.h"
#include "PdfToExcel.h"
#include "ScrapePart1.h"
#include "ExcelExporter1.h"

#include "tools.h"

using namespace std;

class file_data
{
    FILE* m_pData;
public:
    file_data(string fn)
        : m_pData(nullptr)
    {
        fopen_s(&m_pData, fn.c_str(), "rb");
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

#include <filesystem>

int main(int argc, char* argv[])
{
    string base_dir;
    if (argc > 1) {
        string tmp = argv[1];
        if (tmp.back() == '\\' || tmp.back() == '\"') {
            tmp.erase(tmp.size() - 1, 1);
        }
        std::experimental::filesystem::path file(tmp);
        base_dir = file.string();
        if (base_dir.empty()) {
            printf("Path not found %s\n", argv[1]);
            std::experimental::filesystem::path file_backup(argv[0]);
            base_dir = file_backup.parent_path().string();
        }
    } else {
        std::experimental::filesystem::path file(argv[0]);
        base_dir = file.parent_path().string();
    }

    for (auto& p : std::experimental::filesystem::directory_iterator(base_dir)) {
        std::experimental::filesystem::path file(p);
        if (file.extension() == L".pdf" || file.extension() == L".PDF") {
            string sName = file.string();
            printf("Converting PDF '%s'\n", sName.c_str());
            file_data fd(sName);
            fd.ReadAllData();
            CPdfDocument* document = new CPdfDocument(sName, std::move(fd.m_Data));
            document->Initialize();
            
            /*CPdfToExcel pdf2excel;
            if (pdf2excel.ProcessDocument(document) == false) {
                printf("Could not convert '%s'\n", sName.c_str());
            }*/
            
            /*CScrapePart1 scraper;
            scraper.ProcessDocument(document);*/
            
            CExcelExporter1 exp;
            exp.ProcessDocument(document);


            delete document;
        }
    }
   
    printf("End of conversion\n");
    return 0;
}




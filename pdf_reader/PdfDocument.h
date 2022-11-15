#pragma once

#include "PdfTypes.h"

class CPdfObject;
class CPdfDictionary;
class CPdfFont;
class CPdfObject;

class CPdfDocument
{
    std::vector<char> m_Data;
    std::vector<uint32_t> m_ChapterOffset;
    std::map<std::string, CPdfObject*> m_ObjectData;

    std::vector<TextAtLocation> m_AllTexts;
    std::vector<VerticalLine> m_AllVerticalLines;
    std::vector<HorizontalLine> m_AllHorizontalLines;
    std::string m_sFilename;
    std::vector<std::string> m_MediaBoxes;

    std::vector<CPdfObject*> m_AllObjects;

    

public:
    CPdfDocument() = delete;
    CPdfDocument(std::string sFilename, std::vector<char>&& data);
    ~CPdfDocument();

    void ProcessObject(int object);
    void Initialize();
    void CollectAllTextNodes(std::function<bool(const TextAtLocation&, const TextAtLocation&)> sort_function);
    
    CPdfObject* GetObjectData(int object);
    CPdfObject* GetObjectData(std::string object);

    void EnsureChapterSize(size_t count);
    void SetChapterIndex(size_t object, size_t offset);

    void ProcessInternalObject(int object, char* data, char* end_data);

    const std::vector<TextAtLocation>& GetAllTextNodes() const {
        return m_AllTexts;
    }

    const std::vector<VerticalLine>& GetAllVerticalLines() const {
        return m_AllVerticalLines;
    }

    const std::vector<HorizontalLine>& GetAllHorizontalLines() const {
        return m_AllHorizontalLines;
    }

    const std::vector<std::string>& GetAllMediaBoxes() const {
        return m_MediaBoxes;
    }

    const std::string& GetFilename() const { return m_sFilename; }

    void AddObject(CPdfObject* pObject);

private:
    void Process();
    void ReadXRef(int offset);
    CPdfObject* ProcessType(CPdfDictionary* pDict, char* data);
    void ProcessCustomLine(const DIGraphicsState& GS, const PathState& PS, const PdfPoint& point, size_t nPage);
};


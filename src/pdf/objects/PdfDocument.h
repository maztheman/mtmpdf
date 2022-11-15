#pragma once

#include "pdf/types/Line.h"
#include "pdf/types/PathState.h"
#include "pdf/types/TextAtLocation.h"

class CPdfObject;
class CPdfDictionary;
class CPdfFont;
class CPdfObject;
class CPdfPage;


struct SChapterInfo
{
    uint32_t offset;
    uint32_t generation;
    char flag;
};

class CPdfDocument
{
    std::vector<char> m_Data;
    std::vector<uint32_t> m_ChapterOffset;
    std::vector<SChapterInfo> m_ChapterGeneration;
    std::map<std::string, CPdfObject*> m_ObjectData;

    std::string m_sFilename;
    std::vector<std::string> m_MediaBoxes;

    std::vector<CPdfObject*> m_AllObjects;
public:
    CPdfDocument() = delete;
    CPdfDocument(std::string sFilename, std::vector<char>&& data);
    ~CPdfDocument();

    void ProcessObject(int object, bool bDoNotProcessIfBroken = false);
    void Initialize();

    std::vector<std::tuple<CPdfPage*, std::string>> GetPageContents();
    
    CPdfObject* GetObjectData(int object);
    CPdfObject* GetObjectData(std::string object);

    void EnsureChapterSize(size_t count);
    void SetChapterIndex(size_t object, uint32_t offset, uint32_t generation, char flag);

    void ProcessInternalObject(int object, char* data, char* end_data);

    const std::vector<std::string>& GetAllMediaBoxes() const {
        return m_MediaBoxes;
    }

    const std::string& GetFilename() const { return m_sFilename; }

    void AddObject(CPdfObject* pObject);

    size_t GetObjectCount() const {
        return m_ChapterOffset.size();
    }

    const std::vector<CPdfObject*>& GetAllObjects() const {
        return m_AllObjects;
    }
private:
    void Process();
    void ReadXRef(int offset);
    CPdfObject* ProcessType(CPdfDictionary* pDict, char* data);
};
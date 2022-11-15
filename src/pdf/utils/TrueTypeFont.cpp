#include "TrueTypeFont.h"

#include "tools.h"

#include "pdf/objects/PdfDocument.h"
#include "pdf/objects/PdfDictionary.h"

#include <string_view>

using namespace std;

#define LOBYTE(w) ((uint8_t) (w))
#define HIBYTE(w) ((uint8_t) (((uint16_t) (w) >> 8) & 0xFF))

#define LOWORD(x) ((uint16_t) (x))
#define HIWORD(x) ((uint16_t) (((uint32_t) (x) >> 16) & 0xFFFF))


#define MAKEWORD(l,h) ((uint8_t)(l) | ((uint8_t)(h) << 8))
#define MAKELONG(l,h) ((uint16_t)(l) | ((uint16_t)(h) << 16)) 
#define SWAPWORD(x) MAKEWORD(LOBYTE(x), HIBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

struct FIXED
{
    uint16_t fract;
    int16_t value;
};

typedef struct true_type_offset_tbl_s
{
    uint16_t uMajorVersion;
    uint16_t uMinorVersion;
    uint16_t uNumOfTables;
    uint16_t uSearchRange;
    uint16_t uEntrySelector;
    uint16_t uRangeShift;
} true_type_offset_tbl_t;

//Tables in TTF file and there placement and name (tag)
typedef struct true_type_table_dir_s
{
    char szTag[4]; //table name
    uint32_t uCheckSum; //Check sum
    uint32_t uOffset; //Offset from beginning of file
    uint32_t uLength; //length of the table in uint8_ts
} true_type_table_dir_t;

//Header of names table
typedef struct true_type_name_tbl_hdr_s
{
    uint16_t uFSelector; //format selector. Always 0
    uint16_t uNRCount; //Name Records count
    uint16_t uStorageOffset; //Offset for strings storage, 
                           //from start of the table
} true_type_name_tbl_hdr_t;

//Record in names table
typedef struct true_type_name_rs_s
{
    uint16_t uPlatformID;
    uint16_t uEncodingID;
    uint16_t uLanguageID;
    uint16_t uNameID;
    uint16_t uStringLength;
    uint16_t uStringOffset; //from start of storage area
} true_type_name_rs_t;

typedef struct true_type_hhea_tbl_hdr_s
{
    FIXED uTableVersion;
    int16_t fwAscender;
    int16_t fwDescender;
    int16_t fwLineGap;
    uint16_t ufwAdvancedWidthMax;
    int16_t fwMinLeftSideBearing;
    int16_t fwMinRightSideBearing;
    int16_t fwxMaxExtent;
    int16_t nCaretSlopeRise;
    int16_t nCaretSlopRun;
    int16_t nReserved1;
    int16_t nReserved2;
    int16_t nReserved3;
    int16_t nReserved4;
    int16_t nReserved5;
    int16_t nMetricDataFormat;
    uint16_t nNumberOfHMetrics;
} true_type_hhea_tbl_hdr_t;

typedef struct true_type_head_tbl_hdr_s
{
    FIXED uTableVersion;
    FIXED uFontRevision;
    uint32_t uCheckSumAdjustment;
    uint32_t uMagicNumber;
    uint16_t uFlags;
    uint16_t uUnitsPerEm;
    uint32_t dtCreated[2];
    uint32_t dtModified[2];
    int16_t fwXMin;
    int16_t fwYMin;
    int16_t fwXMax;
    int16_t fwYMax;
    uint16_t uMacStyle;
    uint16_t uLowestRecPPEM;
    int16_t nFontDirectionHint;
    int16_t nIndxToToLocFormat;
    int16_t nGlyphDataFormat;
} true_type_head_tbl_hdr_t;

typedef struct true_type_maxp_tbl_hdr_s
{
    FIXED  uTableVersion;
    uint16_t uNumGlyphs;
    uint16_t uMaxPoints;
    uint16_t uMaxContours;
    uint16_t uMaxCompositePoints;
    uint16_t uMaxCompositeContours;
    uint16_t uMaxZones;
    uint16_t uMaxTwilightPoints;
    uint16_t uMaxStorage;
    uint16_t uMaxFunctionDefs;
    uint16_t uMaxIntructionDefs;
    uint16_t uMaxStackElements;
    uint16_t uMaxSizeOfInstructions;
    uint16_t uMaxComponentElements;
    uint16_t uMaxComponentDepth;
} true_type_maxp_tbl_hdr_t;


typedef struct true_type_cmap_tbl_hdr_s
{
    uint16_t uTableVersion;
    uint16_t nNumberOfEncTables;
} true_type_cmap_tbl_hdr_t;

typedef struct true_type_encoding_tbl_s
{
    uint16_t uPlatformID;
    uint16_t uEncodingID;
    uint32_t uOffsetEncodingSubTable;
} true_type_encoding_tbl_t;


typedef struct true_type_hor_mertric_s
{
    uint16_t ufwAdvancedWidth;
    int16_t fwLSB;
} true_type_hor_mertric_t;

typedef struct true_type_encoding_subtbl_s
{
    uint16_t uFormat;
    uint16_t uLength;
    uint16_t uVersion;
} true_type_encoding_subtbl_t;


template<typename T>
static inline void read_data(T& retval, const char*& val)
{
    memcpy(&retval, val, sizeof(T));
    val += sizeof(T);
}

static inline uint8_t read_uint8_t(const char*& val)
{
    uint8_t retval;
    read_data(retval, val);
    return retval;
}


static inline uint16_t read_uint16_t(const char*& val)
{
    uint16_t retval;
    read_data(retval, val);
    return SWAPWORD(retval);
}

CTrueTypeFont::CTrueTypeFont(CPdfDocument* pDocument)
    : CPdfObject(pDocument)
{
}


CTrueTypeFont::~CTrueTypeFont()
{
}

void CTrueTypeFont::Install()
{
}

string GetFontName(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    string sName;

    true_type_table_dir_t tbl_dir;
    bool bFound = false;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        //table's tag cannot exceed 4 characters
        std::string_view table_tag = tbl_dir.szTag;

        if (table_tag == "name") {
            //we found our table. Rearrange order and quit the loop
            bFound = true;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    if (bFound) {
        //move to offset we got from Offsets Table
        file_data = &origin[tbl_dir.uOffset];

        true_type_name_tbl_hdr_t name_tbl_hdr;
        memcpy(&name_tbl_hdr, file_data, sizeof(true_type_name_tbl_hdr_t));
        file_data += sizeof(true_type_name_tbl_hdr_t);

        //again, don't forget to swap uint8_ts!
        name_tbl_hdr.uNRCount = SWAPWORD(name_tbl_hdr.uNRCount);
        name_tbl_hdr.uStorageOffset = SWAPWORD(name_tbl_hdr.uStorageOffset);
        true_type_name_rs_t rs;
        bFound = false;

        for (int i = 0; i < name_tbl_hdr.uNRCount; i++) {
            memcpy(&rs, file_data, sizeof(true_type_name_rs_t));
            file_data += sizeof(true_type_name_rs_t);

            rs.uNameID = SWAPWORD(rs.uNameID);
            //1 says that this is font name. 0 for example determines copyright info
            if (rs.uNameID == 1 || (rs.uNameID == 6 && name_tbl_hdr.uNRCount == 1)) {
                rs.uPlatformID = SWAPWORD(rs.uPlatformID);
                rs.uEncodingID = SWAPWORD(rs.uEncodingID);
                rs.uStringLength = SWAPWORD(rs.uStringLength);
                rs.uStringOffset = SWAPWORD(rs.uStringOffset);
                const char* pPos = &origin[tbl_dir.uOffset + rs.uStringOffset + name_tbl_hdr.uStorageOffset];
                std::vector<char> buff(rs.uStringLength, 0);
                memcpy(&buff[0], pPos, rs.uStringLength);
                if (rs.uPlatformID == 1 || rs.uPlatformID == 2) {
                    sName = string(buff.begin(), buff.end());
                } else {
                    vector<wchar_t> buff2;
                    buff2.reserve(rs.uStringLength);
                    //Microsoft, Unicode
                    for (size_t n = 0; n < buff.size(); n += 2) {
                        std::swap(buff[n], buff[n + 1]);
                        buff2.push_back(buff[n] | (buff[n + 1] << 8));
                    }

                    if (wcsnlen(&buff2[0], rs.uStringLength) > 0) {
                        sName = str_from_wstr(buff2);
                        break;
                    }
                    //End Microsoft Unicode
                }
                
            }
        }
    }

    return sName;
}

uint16_t GetNumberOfHMetrics(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    true_type_table_dir_t tbl_dir;
    bool bFound = false;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        std::string_view table_tag = tbl_dir.szTag;
        if (table_tag == "hhea") 
        {
            //we found our table. Rearrange order and quit the loop
            bFound = true;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    true_type_hhea_tbl_hdr_t hhea_tbl_hdr;
    uint16_t retval = 0;

    if (bFound) {
        file_data = &origin[tbl_dir.uOffset];
        memcpy(&hhea_tbl_hdr, file_data, sizeof(true_type_hhea_tbl_hdr_t));
        retval = hhea_tbl_hdr.nNumberOfHMetrics = SWAPWORD(hhea_tbl_hdr.nNumberOfHMetrics);
    }

    return retval;
}



uint16_t GetUnitsPerEM(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    true_type_table_dir_t tbl_dir;
    bool bFound = false;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        std::string_view table_tag = tbl_dir.szTag;
        if (table_tag == "head")
        {
            //we found our table. Rearrange order and quit the loop
            bFound = true;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    true_type_head_tbl_hdr_t head_tbl_hdr = { 0 };
    uint16_t retval = 0;

    if (bFound) {
        file_data = &origin[tbl_dir.uOffset];
        read_data(head_tbl_hdr, file_data);
        head_tbl_hdr.uUnitsPerEm = SWAPWORD(head_tbl_hdr.uUnitsPerEm);
    }

    return head_tbl_hdr.uUnitsPerEm;
}


true_type_maxp_tbl_hdr_t GetMaxPTable(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    true_type_table_dir_t tbl_dir;
    bool bFound = false;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        std::string_view table_tag = tbl_dir.szTag;
        if (table_tag == "maxp")
        {
            //we found our table. Rearrange order and quit the loop
            bFound = true;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    true_type_maxp_tbl_hdr_t retval = { 0 };

    if (bFound) {
        file_data = &origin[tbl_dir.uOffset];
        memcpy(&retval, file_data, sizeof(true_type_maxp_tbl_hdr_t));
        //retval.uTableVersion = SWAPLONG(retval.uTableVersion);
        retval.uNumGlyphs = SWAPWORD(retval.uNumGlyphs);
        retval.uMaxPoints = SWAPWORD(retval.uMaxPoints);
        retval.uMaxContours = SWAPWORD(retval.uMaxContours);
        retval.uMaxCompositePoints = SWAPWORD(retval.uMaxCompositePoints);
        retval.uMaxCompositeContours = SWAPWORD(retval.uMaxCompositeContours);
        retval.uMaxZones = SWAPWORD(retval.uMaxZones);
        retval.uMaxTwilightPoints = SWAPWORD(retval.uMaxTwilightPoints);
        retval.uMaxStorage = SWAPWORD(retval.uMaxStorage);
        retval.uMaxFunctionDefs = SWAPWORD(retval.uMaxFunctionDefs);
        retval.uMaxIntructionDefs = SWAPWORD(retval.uMaxIntructionDefs);
        retval.uMaxStackElements = SWAPWORD(retval.uMaxStackElements);
        retval.uMaxSizeOfInstructions = SWAPWORD(retval.uMaxSizeOfInstructions);
        retval.uMaxComponentElements = SWAPWORD(retval.uMaxComponentElements);
        retval.uMaxComponentDepth = SWAPWORD(retval.uMaxComponentDepth);
    }

    return retval;
}





map<char, int> GetCMapTable(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    map<char, int> retval;
    true_type_table_dir_t tbl_dir;
    bool bFound = false;

    for (int i = 0; i < off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        std::string_view table_tag = tbl_dir.szTag;
        if (table_tag == "cmap")
        {
            //we found our table. Rearrange order and quit the loop
            bFound = true;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    if (bFound) {
        auto table_dir_offset = &origin[tbl_dir.uOffset];
        file_data = table_dir_offset;
        true_type_cmap_tbl_hdr_t cmap_tbl_hdr;
        memcpy(&cmap_tbl_hdr, file_data, sizeof(true_type_cmap_tbl_hdr_t));
        file_data += sizeof(true_type_cmap_tbl_hdr_t);
        cmap_tbl_hdr.nNumberOfEncTables = SWAPWORD(cmap_tbl_hdr.nNumberOfEncTables);
        vector<true_type_encoding_tbl_t> encoding_tables(cmap_tbl_hdr.nNumberOfEncTables);
        for (int i = 0; i < cmap_tbl_hdr.nNumberOfEncTables; i++) {
            auto& enc_tbl = encoding_tables[i];
            memcpy(&enc_tbl, file_data, sizeof(true_type_encoding_tbl_t));
            enc_tbl.uEncodingID = SWAPWORD(enc_tbl.uEncodingID);
            enc_tbl.uOffsetEncodingSubTable = SWAPLONG(enc_tbl.uOffsetEncodingSubTable);
            enc_tbl.uPlatformID = SWAPWORD(enc_tbl.uPlatformID);
            file_data += sizeof(true_type_encoding_tbl_t);
        }

        for (auto& enc_tbl : encoding_tables) {
            file_data = table_dir_offset + enc_tbl.uOffsetEncodingSubTable;
            true_type_encoding_subtbl_t subtbl = { 0 };

            read_data(subtbl, file_data);

            subtbl.uFormat = SWAPWORD(subtbl.uFormat);
            subtbl.uLength = SWAPWORD(subtbl.uLength);
            subtbl.uVersion = SWAPWORD(subtbl.uVersion);

            if (subtbl.uFormat == 0) {
                //uint8_t formatting, easy mode
                for (int i = 0; i < 256; i++) {
                    uint8_t glyph;
                    read_data(glyph, file_data);
                    retval.emplace((char)i, glyph);
                }

            } else if (subtbl.uFormat == 4) {
                uint16_t uSegCountX2 = read_uint16_t(file_data);
                uint16_t uSearchRange = read_uint16_t(file_data);
                uint16_t uEntrySelector = read_uint16_t(file_data);
                uint16_t uRangeShift = read_uint16_t(file_data);
                uint16_t uSegCount = uSegCountX2 >> 1;

                vector<uint16_t> arStartCode(uSegCount);
                vector<uint16_t> arEndCode(uSegCount);
                vector<uint16_t> arIdDelta(uSegCount);
                vector<uint16_t> arIdOffsetRange(uSegCount);

                for (auto& ec : arEndCode) {
                    ec = read_uint16_t(file_data);
                }
                file_data += sizeof(uint16_t);
                for (auto& sc : arStartCode) {
                    sc = read_uint16_t(file_data);
                }

                for (auto& idd : arIdDelta) {
                    idd = read_uint16_t(file_data);
                }

                const char* id_offset_range_ptr = file_data;

                for (auto& ioff : arIdOffsetRange) {
                    ioff = read_uint16_t(file_data);
                }

                for (int c = 0; c < 256; c++) {
                    for (int n = 0; n < uSegCount; n++) {
                        if (arEndCode[n] >= c && arStartCode[n] <= c) {
                            if (arIdOffsetRange[n] == 0) {
                                int glyph = (arIdDelta[n] + c) & 65535;
                                retval.emplace((char)c, glyph);
                            } else {
                                auto ptr = (arIdOffsetRange[n] / 2 + (c - arStartCode[n]) + id_offset_range_ptr);
                                int glyph = read_uint16_t(ptr);
                                retval.emplace((char)c, glyph);
                            }

                        }
                    }
                }
            } else if (subtbl.uFormat == 6) {
                uint16_t uFirstCode = read_uint16_t(file_data);
                uint16_t uEntryCount = read_uint16_t(file_data);
                uint16_t uLastCode = uFirstCode + uEntryCount;
                vector<uint16_t> arGlyphIndexArray(uEntryCount);
                for (int n = 0; n < uEntryCount; n++) {
                    arGlyphIndexArray[n] = read_uint16_t(file_data);
                }
                for (int c = uFirstCode; c < uLastCode; c++) {
                    int glyph_idx = c - uFirstCode;
                    retval.emplace((char)c, arGlyphIndexArray[glyph_idx]);
                }
            }
            int h = 0;
        }
    }

    return retval;
}



SFontWidthData GetWidthData(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    SFontWidthData retval;


    auto nNumberOfMetrics = GetNumberOfHMetrics(off_tbl, file_data, origin);
    auto maxp_tbl = GetMaxPTable(off_tbl, file_data, origin);
    retval.CMap = GetCMapTable(off_tbl, file_data, origin);
    retval.UnitsPerEM = GetUnitsPerEM(off_tbl, file_data, origin);

    true_type_table_dir_t tbl_dir;
    bool bFound = false;
    
    for (int i = 0; i< off_tbl.uNumOfTables; i++)
    {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        std::string_view table_tag = tbl_dir.szTag;
        if (table_tag == "hmtx")
        {
            //we found our table. Rearrange order and quit the loop
            bFound = true;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    if (bFound) {
        file_data = &origin[tbl_dir.uOffset];
        true_type_hor_mertric_t rs;
        retval.GlyphWidths.resize(nNumberOfMetrics);
        for (int i = 0; i < nNumberOfMetrics; i++) {
            memcpy(&rs, file_data, sizeof(true_type_hor_mertric_t));
            file_data += sizeof(true_type_hor_mertric_t);
            rs.ufwAdvancedWidth = SWAPWORD(rs.ufwAdvancedWidth);
            retval.GlyphWidths[i] = rs.ufwAdvancedWidth;
        }
    }

    /*
    For example, assume that a glyph feature is 550 FUnits in length on a 72 dpi screen at 18 point. There are 2048 units per em. The following calculation reveals that the feature is 4.83 pixels long.
    550 * 18 * 72 / ( 72 * 2048 ) = 4.83
    */

    return retval;
}


CTrueTypeFont* CTrueTypeFont::FromMemory(CPdfDocument* pDocument, const std::string& data)
{
    const char* file_data = &data[0];
    const char* origin = file_data;

    string sName;

    uint32_t header = 0;
    memcpy(&header, file_data, 4);

    true_type_offset_tbl_t off_tbl;
    memcpy(&off_tbl, file_data, sizeof(true_type_offset_tbl_t));
    file_data += sizeof(true_type_offset_tbl_t);

    off_tbl.uMajorVersion = SWAPWORD(off_tbl.uMajorVersion);
    off_tbl.uMinorVersion = SWAPWORD(off_tbl.uMinorVersion);
    off_tbl.uNumOfTables = SWAPWORD(off_tbl.uNumOfTables);
    
    if ((off_tbl.uMajorVersion != 1 || off_tbl.uMinorVersion != 0) &&
        (off_tbl.uMajorVersion != 0x7472 || off_tbl.uMinorVersion != 0x7565))
        return nullptr;

    sName = GetFontName(off_tbl, file_data, origin);
    auto wd = GetWidthData(off_tbl, file_data, origin);

    if (sName.empty() == false) {
        CTrueTypeFont* pFnt = new CTrueTypeFont(pDocument);
        pFnt->m_sName = sName;
        pFnt->m_Data = data;
        pFnt->m_Widths = wd;
        return pFnt;
    }
    
    return nullptr;
}

int CTrueTypeFont::GetGlyphWidth(char c)
{
    auto it = m_Widths.CMap.find(c);
    if (it == m_Widths.CMap.end()) {
        return 500;
    }

    if (it->second >= (int)m_Widths.GlyphWidths.size()) {
        return m_Widths.GlyphWidths.back();
    }

    return m_Widths.GlyphWidths[it->second];
}
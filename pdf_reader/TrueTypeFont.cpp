#include "stdafx.h"
#include "TrueTypeFont.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"

using namespace std;

#define SWAPWORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define SWAPLONG(x) MAKELONG(SWAPWORD(HIWORD(x)), SWAPWORD(LOWORD(x)))

typedef struct true_type_offset_tbl_s
{
    USHORT uMajorVersion;
    USHORT uMinorVersion;
    USHORT uNumOfTables;
    USHORT uSearchRange;
    USHORT uEntrySelector;
    USHORT uRangeShift;
} true_type_offset_tbl_t;

//Tables in TTF file and there placement and name (tag)
typedef struct true_type_table_dir_s
{
    char szTag[4]; //table name
    ULONG uCheckSum; //Check sum
    ULONG uOffset; //Offset from beginning of file
    ULONG uLength; //length of the table in bytes
} true_type_table_dir_t;

//Header of names table
typedef struct true_type_name_tbl_hdr_s
{
    USHORT uFSelector; //format selector. Always 0
    USHORT uNRCount; //Name Records count
    USHORT uStorageOffset; //Offset for strings storage, 
                           //from start of the table
} true_type_name_tbl_hdr_t;

//Record in names table
typedef struct true_type_name_rs_s
{
    USHORT uPlatformID;
    USHORT uEncodingID;
    USHORT uLanguageID;
    USHORT uNameID;
    USHORT uStringLength;
    USHORT uStringOffset; //from start of storage area
} true_type_name_rs_t;

typedef struct true_type_hhea_tbl_hdr_s
{
    FIXED uTableVersion;
    SHORT fwAscender;
    SHORT fwDescender;
    SHORT fwLineGap;
    USHORT ufwAdvancedWidthMax;
    SHORT fwMinLeftSideBearing;
    SHORT fwMinRightSideBearing;
    SHORT fwxMaxExtent;
    SHORT nCaretSlopeRise;
    SHORT nCaretSlopRun;
    SHORT nReserved1;
    SHORT nReserved2;
    SHORT nReserved3;
    SHORT nReserved4;
    SHORT nReserved5;
    SHORT nMetricDataFormat;
    USHORT nNumberOfHMetrics;
} true_type_hhea_tbl_hdr_t;

typedef struct true_type_head_tbl_hdr_s
{
    FIXED uTableVersion;
    FIXED uFontRevision;
    ULONG uCheckSumAdjustment;
    ULONG uMagicNumber;
    USHORT uFlags;
    USHORT uUnitsPerEm;
    ULONG dtCreated[2];
    ULONG dtModified[2];
    SHORT fwXMin;
    SHORT fwYMin;
    SHORT fwXMax;
    SHORT fwYMax;
    USHORT uMacStyle;
    USHORT uLowestRecPPEM;
    SHORT nFontDirectionHint;
    SHORT nIndxToToLocFormat;
    SHORT nGlyphDataFormat;
} true_type_head_tbl_hdr_t;

typedef struct true_type_maxp_tbl_hdr_s
{
    FIXED  uTableVersion;
    USHORT uNumGlyphs;
    USHORT uMaxPoints;
    USHORT uMaxContours;
    USHORT uMaxCompositePoints;
    USHORT uMaxCompositeContours;
    USHORT uMaxZones;
    USHORT uMaxTwilightPoints;
    USHORT uMaxStorage;
    USHORT uMaxFunctionDefs;
    USHORT uMaxIntructionDefs;
    USHORT uMaxStackElements;
    USHORT uMaxSizeOfInstructions;
    USHORT uMaxComponentElements;
    USHORT uMaxComponentDepth;
} true_type_maxp_tbl_hdr_t;


typedef struct true_type_cmap_tbl_hdr_s
{
    USHORT uTableVersion;
    USHORT nNumberOfEncTables;
} true_type_cmap_tbl_hdr_t;

typedef struct true_type_encoding_tbl_s
{
    USHORT uPlatformID;
    USHORT uEncodingID;
    ULONG uOffsetEncodingSubTable;
} true_type_encoding_tbl_t;


typedef struct true_type_hor_mertric_s
{
    USHORT ufwAdvancedWidth;
    SHORT fwLSB;
} true_type_hor_mertric_t;

typedef struct true_type_encoding_subtbl_s
{
    USHORT uFormat;
    USHORT uLength;
    USHORT uVersion;
} true_type_encoding_subtbl_t;


template<typename T>
static inline void read_data(T& retval, const char*& val)
{
    memcpy(&retval, val, sizeof(T));
    val += sizeof(T);
}

static inline BYTE read_byte(const char*& val)
{
    BYTE retval;
    read_data(retval, val);
    return retval;
}


static inline USHORT read_ushort(const char*& val)
{
    USHORT retval;
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
    BOOL bFound = FALSE;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        char table_tag[5] = { 0 };
        //table's tag cannot exceed 4 characters
        strncpy_s(table_tag, tbl_dir.szTag, 4);
        if (_strnicmp(table_tag, "name", 4) == 0) {
            //we found our table. Rearrange order and quit the loop
            bFound = TRUE;
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

        //again, don't forget to swap bytes!
        name_tbl_hdr.uNRCount = SWAPWORD(name_tbl_hdr.uNRCount);
        name_tbl_hdr.uStorageOffset = SWAPWORD(name_tbl_hdr.uStorageOffset);
        true_type_name_rs_t rs;
        bFound = FALSE;

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
                vector<char> buff(rs.uStringLength, 0);
                memcpy(&buff[0], pPos, rs.uStringLength);
                if (rs.uPlatformID == 1 || rs.uPlatformID == 2) {
                    sName = string(buff.begin(), buff.end());
                } else {
                    vector<wchar_t> buff2;
                    buff2.reserve(rs.uStringLength);
                    //Microsoft, Unicode
                    for (size_t n = 0; n < buff.size(); n += 2) {
                        swap(buff[n], buff[n + 1]);
                        buff2.push_back(buff[n] | (buff[n + 1] << 8));
                    }

                    if (wcsnlen_s(&buff2[0], rs.uStringLength) > 0) {
                        sName = string(buff2.begin(), buff2.end());
                        break;
                    }
                }
                //End Microsoft Unicode
            }
        }
    }

    return sName;
}

USHORT GetNumberOfHMetrics(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    true_type_table_dir_t tbl_dir;
    BOOL bFound = FALSE;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        char table_tag[5] = { 0 };
        //table's tag cannot exceed 4 characters
        strncpy_s(table_tag, tbl_dir.szTag, 4);
        if (_strnicmp(table_tag, "hhea", 4) == 0) {
            //we found our table. Rearrange order and quit the loop
            bFound = TRUE;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    true_type_hhea_tbl_hdr_t hhea_tbl_hdr;
    USHORT retval = 0;

    if (bFound) {
        file_data = &origin[tbl_dir.uOffset];
        memcpy(&hhea_tbl_hdr, file_data, sizeof(true_type_hhea_tbl_hdr_t));
        retval = hhea_tbl_hdr.nNumberOfHMetrics = SWAPWORD(hhea_tbl_hdr.nNumberOfHMetrics);
    }

    return retval;
}



USHORT GetUnitsPerEM(true_type_offset_tbl_t& off_tbl, const char* file_data, const char* origin)
{
    true_type_table_dir_t tbl_dir;
    BOOL bFound = FALSE;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        char table_tag[5] = { 0 };
        //table's tag cannot exceed 4 characters
        strncpy_s(table_tag, tbl_dir.szTag, 4);
        if (_strnicmp(table_tag, "head", 4) == 0) {
            //we found our table. Rearrange order and quit the loop
            bFound = TRUE;
            tbl_dir.uLength = SWAPLONG(tbl_dir.uLength);
            tbl_dir.uOffset = SWAPLONG(tbl_dir.uOffset);
            break;
        }
    }

    true_type_head_tbl_hdr_t head_tbl_hdr = { 0 };
    USHORT retval = 0;

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
    BOOL bFound = FALSE;

    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        char table_tag[5] = { 0 };
        //table's tag cannot exceed 4 characters
        strncpy_s(table_tag, tbl_dir.szTag, 4);
        if (_strnicmp(table_tag, "maxp", 4) == 0) {
            //we found our table. Rearrange order and quit the loop
            bFound = TRUE;
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
    BOOL bFound = FALSE;

    for (int i = 0; i < off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        char table_tag[5] = { 0 };
        //table's tag cannot exceed 4 characters
        strncpy_s(table_tag, tbl_dir.szTag, 4);
        if (_strnicmp(table_tag, "cmap", 4) == 0) {
            //we found our table. Rearrange order and quit the loop
            bFound = TRUE;
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
                //byte formatting, easy mode
                for (int i = 0; i < 256; i++) {
                    BYTE glyph;
                    read_data(glyph, file_data);
                    retval.emplace((char)i, glyph);
                }

            } else if (subtbl.uFormat == 4) {
                USHORT uSegCountX2 = read_ushort(file_data);
                USHORT uSearchRange = read_ushort(file_data);
                USHORT uEntrySelector = read_ushort(file_data);
                USHORT uRangeShift = read_ushort(file_data);
                USHORT uSegCount = uSegCountX2 >> 1;

                vector<USHORT> arStartCode(uSegCount);
                vector<USHORT> arEndCode(uSegCount);
                vector<USHORT> arIdDelta(uSegCount);
                vector<USHORT> arIdOffsetRange(uSegCount);

                for (auto& ec : arEndCode) {
                    ec = read_ushort(file_data);
                }
                file_data += sizeof(USHORT);
                for (auto& sc : arStartCode) {
                    sc = read_ushort(file_data);
                }

                for (auto& idd : arIdDelta) {
                    idd = read_ushort(file_data);
                }

                const char* id_offset_range_ptr = file_data;

                for (auto& ioff : arIdOffsetRange) {
                    ioff = read_ushort(file_data);
                }

                for (int c = 0; c < 256; c++) {
                    for (int n = 0; n < uSegCount; n++) {
                        if (arEndCode[n] >= c && arStartCode[n] <= c) {
                            if (arIdOffsetRange[n] == 0) {
                                int glyph = (arIdDelta[n] + c) & 65535;
                                retval.emplace((char)c, glyph);
                            } else {
                                auto ptr = (arIdOffsetRange[n] / 2 + (c - arStartCode[n]) + id_offset_range_ptr);
                                int glyph = read_ushort(ptr);
                                retval.emplace((char)c, glyph);
                            }

                        }
                    }
                }
            } else if (subtbl.uFormat == 6) {
                USHORT uFirstCode = read_ushort(file_data);
                USHORT uEntryCount = read_ushort(file_data);
                USHORT uLastCode = uFirstCode + uEntryCount;
                vector<USHORT> arGlyphIndexArray(uEntryCount);
                for (int n = 0; n < uEntryCount; n++) {
                    arGlyphIndexArray[n] = read_ushort(file_data);
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
    BOOL bFound = FALSE;
    
    for (int i = 0; i< off_tbl.uNumOfTables; i++) {
        memcpy(&tbl_dir, file_data, sizeof(true_type_table_dir_t));
        file_data += sizeof(true_type_table_dir_t);
        char table_tag[5] = { 0 };
        //table's tag cannot exceed 4 characters
        strncpy_s(table_tag, tbl_dir.szTag, 4);
        if (_strnicmp(table_tag, "hmtx", 4) == 0) {
            //we found our table. Rearrange order and quit the loop
            bFound = TRUE;
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

    ULONG header = 0;
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

    if (it->second >= m_Widths.GlyphWidths.size()) {
        return m_Widths.GlyphWidths.back();
    }

    return m_Widths.GlyphWidths[it->second];
}
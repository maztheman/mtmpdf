#include "zlib_helper.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 262144

using namespace std;

int InflateData(vector<char>& buffer, vector<char>& dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    std::vector<unsigned char> out(CHUNK);
    vector<vector<unsigned char>> out_chunks;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    size_t out_size = 0;
    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = buffer.size();
        if (strm.avail_in == 0)
            break;
        strm.next_in = reinterpret_cast<unsigned char*>(&buffer[0]);

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = &out[0];
            ret = inflate(&strm, Z_NO_FLUSH);
            //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }
            have = CHUNK - strm.avail_out;
            out_size += have;
            out_chunks.emplace_back(vector<unsigned char>(&out[0], &out[0] + have));
            /*
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }*/
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

    dest.resize(out_size);

    size_t off = 0;
    for (auto& c : out_chunks) {
        memcpy(&dest[off], &c[0], c.size());
        off += c.size();
    }

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}


/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int DeflateData(std::vector<char>& buffer, std::vector<char>& dest)
{
    int level = 5;
    int ret;
    unsigned have;
    z_stream strm;
    //std::vector<unsigned char> in(CHUNK);
    std::vector<unsigned char> out(CHUNK);
    vector<vector<unsigned char>> out_chunks;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    size_t out_size = 0;

    /* compress until end of file */
    
    strm.avail_in = buffer.size();// fread(&in[0], 1, CHUNK, source);
    strm.next_in = reinterpret_cast<unsigned char*>(&buffer[0]);

    /* run deflate() on input until output buffer not full, finish
        compression if all of source has been read in */
    do {
        strm.avail_out = CHUNK;
        strm.next_out = &out[0];
        ret = deflate(&strm, Z_FINISH);    /* no bad return value */
        //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        have = CHUNK - strm.avail_out;
        out_size += have;
        out_chunks.emplace_back(vector<unsigned char>(&out[0], &out[0] + have));
        /*
        if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }*/
    } while (strm.avail_out == 0);
    //assert(strm.avail_in == 0);     /* all input will be used */

    /* done when last data in file processed */

    /* clean up and return */
    (void)deflateEnd(&strm);

    dest.resize(out_size);

    size_t off = 0;
    for (auto& c : out_chunks) {
        memcpy(&dest[off], &c[0], c.size());
        off += c.size();
    }

    return Z_OK;
}

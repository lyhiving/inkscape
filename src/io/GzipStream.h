#ifndef __INKSCAPE_IO_GZIPSTREAM_H__
#define __INKSCAPE_IO_GZIPSTREAM_H__
/**
 * Zlib-enabled input and output streams
 *
 * This is a thin wrapper of 'gzstreams', to
 * allow our customizations.
 *
 * Authors:
 *   Bob Jamison <rjamison@titan.com>
 *
 * Copyright (C) 2004 Inkscape.org
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#include "InkscapeStream.h"

namespace Inkscape
{
namespace IO
{

//#########################################################################
//# G Z I P    I N P U T    S T R E A M
//#########################################################################

/**
 * This class is for deflating a gzip-compressed InputStream source
 *
 */
class GzipInputStream : public BasicInputStream
{

public:

    GzipInputStream(InputStream &sourceStream);
    
    virtual ~GzipInputStream();
    
    virtual int available();
    
    virtual void close();
    
    virtual int get();
    
private:

    bool load();
    bool loaded;
    
    long totalIn;
    long totalOut;
    
    unsigned char *outputBuf;

    long outputBufPos;
    long outputBufLen;

}; // class GzipInputStream




//#########################################################################
//# G Z I P    O U T P U T    S T R E A M
//#########################################################################

/**
 * This class is for gzip-compressing data going to the
 * destination OutputStream
 *
 */
class GzipOutputStream : public BasicOutputStream
{

public:

    GzipOutputStream(OutputStream &destinationStream);
    
    virtual ~GzipOutputStream();
    
    virtual void close();
    
    virtual void flush();
    
    virtual void put(int ch);

private:

    std::vector<unsigned char> inputBuf;

    long totalIn;
    long totalOut;
    unsigned long crc;

}; // class GzipOutputStream







} // namespace IO
} // namespace Inkscape


#endif /* __INKSCAPE_IO_GZIPSTREAM_H__ */

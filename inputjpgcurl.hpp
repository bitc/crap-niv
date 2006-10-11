#ifndef NIV_INPUTJPGCURL_HPP__
#define NIV_INPUTJPGCURL_HPP__

extern "C" {
#include <jpeglib.h>
}

namespace NIV
{
    void jpeg_curl_src (j_decompress_ptr cinfo, char *url);
}

#endif


#include <stdio.h>
#include <string.h>

#include <pthread.h>

#include <curl/curl.h>

extern "C" {
#include <jpeglib.h>
}

#include "inputjpgcurl.hpp"

/* TEMPORARY */
CURL* tempeasy;
struct UserDefined {
  FILE *err;         /* the stderr user data goes here */
  void *debugdata;   /* the data that will be passed to fdebug */
  char *errorbuffer; /* store failure messages in here */
};

struct Names {
  struct curl_hash *hostcache;
  enum {
    HCACHE_NONE,    /* not pointing to anything */
    HCACHE_PRIVATE, /* points to our own */
    HCACHE_GLOBAL,  /* points to the (shrug) global one */
    HCACHE_MULTI,   /* points to a shared one in the multi handle */
    HCACHE_SHARED   /* points to a shared one in a shared object */
  } hostcachetype;
};

/*
 * The 'connectdata' struct MUST have all the connection oriented stuff as we
 * may have several simultaneous connections and connection structs in memory.
 *
 * The 'struct UserDefined' must only contain data that is set once to go for
 * many (perhaps) independent connections. Values that are generated or
 * calculated internally for the "session handle" must be defined within the
 * 'struct UrlState' instead.
 */

struct SessionHandle {
  struct Names dns;
  struct Curl_multi *multi;    /* if non-NULL, points to the multi handle
                                  struct of which this "belongs" */
  struct Curl_share *share;    /* Share, handles global variable mutexing */
  struct UserDefined set;      /* values set by the libcurl user */
};


#define CURL_INPUT_BUF_SIZE CURL_MAX_WRITE_SIZE * 2

/* Expanded data source object for curl input */

namespace NIV
{
    typedef struct {
        struct jpeg_source_mgr pub;   /* public fields */

        char* url;                    /* source url */
        JOCTET * buffer;              /* start of buffer */
        unsigned char *download_buffer;
        int download_buffer_size;
        pthread_t download_thread;
        pthread_mutex_t mutex;
        bool done_downloading;
    } curl_source_mgr;

    typedef curl_source_mgr * curl_src_ptr;

    /* Called by curl when new data is downloaded */
    size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
    {
        if(nmemb == 0)
        {
            /* TODO ERROR EMPTY FILE */
            printf("ERROR EMPTY FILE\n");
            return 0;
        }

        curl_src_ptr src = (curl_src_ptr) userp;


        pthread_mutex_lock(&src->mutex);

        while(src->download_buffer_size + size*nmemb > CURL_INPUT_BUF_SIZE)
        {
            pthread_mutex_unlock(&src->mutex);
            printf("*** DOWNLOAD BUFFER FULL: WAITING FOR FILL_INPUT\n");
            pthread_yield();
            pthread_mutex_lock(&src->mutex);
        }

        memcpy(src->download_buffer + src->download_buffer_size, buffer, size*nmemb);
        src->download_buffer_size += size*nmemb;
        pthread_mutex_unlock(&src->mutex);

        printf("downloaded shit: %d %d\n", size, nmemb);
        return size * nmemb;
    }

    void* download_fn(void *userp)
    {
        curl_src_ptr src = (curl_src_ptr) userp;

        CURL *easyhandle = curl_easy_init();
        if (!easyhandle)
            throw 0;

        tempeasy = easyhandle;

        printf("errorbuffer: %4x\n", (unsigned int)(((SessionHandle*)tempeasy)->set.errorbuffer));

        curl_easy_setopt(easyhandle, CURLOPT_URL, src->url);
        curl_easy_setopt(easyhandle, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, src);

        printf("errorbuffer: %4x\n", (unsigned int)(((SessionHandle*)tempeasy)->set.errorbuffer));

        /* this seems to be required to avoid segfauls: */
        //curl_easy_setopt(easyhandle, CURLOPT_PROGRESSFUNCTION, NULL);
        //curl_easy_setopt(easyhandle, CURLOPT_NOSIGNAL, (long)1);
        //curl_easy_setopt(easyhandle, CURLOPT_ERRORBUFFER, NULL);

        printf("starting perform: %s\n", src->url);
        int status = curl_easy_perform(easyhandle);
        printf("download finished: %d\n", status);
        if (status)
        {
            /* download error put stream of EOI markers at end of download buffer */
        }

        printf("errorbuffer: %4x\n", (unsigned int)(((SessionHandle*)easyhandle)->set.errorbuffer));

        /* TODO add constant stream of EOI markers to end of buffer */

        curl_easy_cleanup(easyhandle);

        pthread_mutex_lock(&src->mutex);
        src->done_downloading = true;
        pthread_mutex_unlock(&src->mutex);

        return 0;
    }

    static void init_source(j_decompress_ptr cinfo)
    {
        curl_src_ptr src = (curl_src_ptr) cinfo->src;

        printf("init_source\n");
        printf("  Starting download thread\n");

        if (pthread_create(&src->download_thread, NULL, download_fn, src))
        {
            throw 0;
        }
    }

    static boolean fill_input_buffer(j_decompress_ptr cinfo)
    {
        curl_src_ptr src = (curl_src_ptr) cinfo->src;

        pthread_mutex_lock(&src->mutex);
        if (src->download_buffer_size == 0)
        {
            if(src->done_downloading)
            {
                src->pub.next_input_byte = src->buffer;
                src->pub.bytes_in_buffer = 2;
                src->buffer[0] = (JOCTET) 0xFF;
                src->buffer[1] = (JOCTET) JPEG_EOI;
            }
//            if(src->pub.next_input_byte)
//                printf("suspend: %d %d\n", src->pub.next_input_byte - src->buffer, src->pub.bytes_in_buffer);

            pthread_mutex_unlock(&src->mutex);
            return FALSE;
        }
        else
        {
            src->pub.next_input_byte = src->buffer;
            memcpy(src->buffer, src->download_buffer, src->download_buffer_size);
            src->pub.bytes_in_buffer = src->download_buffer_size;
            src->download_buffer_size = 0;

            pthread_mutex_unlock(&src->mutex);
            return TRUE;
        }
    }

    static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
    {
        curl_src_ptr src = (curl_src_ptr) cinfo->src;

        printf("skipping %ld\n", num_bytes);

        /* don't need to lock the mutex, since the download thread
         * doesn't touch pub */

        if (num_bytes > 0) {
            while (num_bytes > (long) src->pub.bytes_in_buffer) {
                num_bytes -= (long) src->pub.bytes_in_buffer;
                while(fill_input_buffer(cinfo) == FALSE);
            }
            src->pub.next_input_byte += (size_t) num_bytes;
            src->pub.bytes_in_buffer -= (size_t) num_bytes;
        }
    }

    static void term_source(j_decompress_ptr cinfo)
    {
        curl_src_ptr src = (curl_src_ptr) cinfo->src;

        printf("term_source, waiting for join\n");
        pthread_join(src->download_thread, NULL);
        printf("join finished\n");
    }

    void jpeg_curl_src (j_decompress_ptr cinfo, char *url)
    {
        curl_src_ptr src;

        /* The source object and input buffer are made permanent so that a series
         * of JPEG images can be read from the same file by calling jpeg_stdio_src
         * only before the first one.  (If we discarded the buffer at the end of
         * one image, we'd likely lose the start of the next one.)
         * This makes it unsafe to use this manager and a different source
         * manager serially with the same JPEG object.  Caveat programmer.
         */
        if (cinfo->src == NULL) {     /* first time for this JPEG object? */
            cinfo->src = (struct jpeg_source_mgr *)
                (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                        sizeof(curl_source_mgr));
            src = (curl_src_ptr) cinfo->src;
            src->buffer = (JOCTET *)
                (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                        CURL_INPUT_BUF_SIZE * sizeof(JOCTET));
            src->download_buffer = (JOCTET *)
                (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                        CURL_INPUT_BUF_SIZE * sizeof(JOCTET));
        }

        src = (curl_src_ptr) cinfo->src;
        src->pub.init_source = init_source;
        src->pub.fill_input_buffer = fill_input_buffer;
        src->pub.skip_input_data = skip_input_data;
        src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
        src->pub.term_source = term_source;
        src->url = url;
        src->download_buffer_size = 0;
        src->done_downloading = false;
        src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
        src->pub.next_input_byte = NULL; /* until buffer loaded */
        int status = pthread_mutex_init(&src->mutex, NULL);
        if (status)
        {
            throw 0;
        }
    }

}


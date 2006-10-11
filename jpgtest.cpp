#include <stdio.h>

#include <GL/gl.h>

#include <curl/curl.h>

extern "C" {
#include <jpeglib.h>
}

#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/time.h>

#include "Window.hpp"
#include "inputjpgcurl.hpp"

void critical_error(const char* error)
{
    printf("Crticial Error: %s\n", error);
    exit(EXIT_FAILURE);
}

/*
inline void put_pixel(SDL_Surface* screen, int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint8* base_address = (Uint8*)screen->pixels;
    Uint8* pixel = base_address + (y * screen->pitch) + (x * screen->format->BytesPerPixel);
    pixel[0] = b;
    pixel[1] = g;
    pixel[2] = r;
}
*/

struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

class Ticks
{
    long sec, usec;
    Ticks()
    {
        timeval tv;
        gettimeofday(&tv, NULL);
        sec = tv.tv_sec;
        usec = tv.tv_usec;
    }
    public:
    static Ticks now()
    {
        return Ticks();
    }
    long operator-(const Ticks& rhs)
    {
        long dsec = sec - rhs.sec;
        long dusec = usec - rhs.usec;
        return dsec * 1000000 + dusec;
    }
};

long long get_ticks()
{
        timeval tv;
        gettimeofday(&tv, NULL);
        printf("%lld\n", ((long long)tv.tv_sec * 1000000) + tv.tv_usec);
        printf("%f\n", (double)(tv.tv_sec + ((double)tv.tv_usec / (double)1000000)));
        return ((long long)tv.tv_sec * 1000000) + tv.tv_usec;
}

int main(int argc, char *argv[])
{
    char* program_name = argv[0];
    if (argc == 1)
    {
        printf("Usage: %s <filename|url>\n", program_name);
        return 0;
    }

    if(curl_global_init(CURL_GLOBAL_ALL))
        throw 0;

//    char* filename = argv[1];
    char* url = argv[1];

//    FILE* fp = fopen(filename, "rb");

//    if (!fp)
//        critical_error("Couldn't open file");

    /* This struct contains the JPEG decompression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     */
    struct jpeg_decompress_struct cinfo;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct my_error_mgr jerr;
    /* More stuff */
    JSAMPARRAY buffer;            /* Output row buffer */
    int row_stride;               /* physical row width in output buffer */

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jpeg_destroy_decompress(&cinfo);
//        fclose(fp);
        printf("ERROR\n");
        return 0;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */

    //jpeg_stdio_src(&cinfo, fp);
    NIV::jpeg_curl_src(&cinfo, url);

    /* Step 3: read file parameters with jpeg_read_header() */

    while(jpeg_read_header(&cinfo, TRUE) == JPEG_SUSPENDED);

    /* Step 4: set parameters for decompression */

    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */

    /* Step 5: Start decompressor */

    cinfo.buffered_image = TRUE;
    //cinfo.enable_1pass_quant = TRUE;
    //cinfo.enable_2pass_quant = TRUE;

    while(jpeg_start_decompress(&cinfo) == FALSE);

    /* We may need to do some setup of our own at this point before reading
     * the data.  After jpeg_start_decompress() we have the correct scaled
     * output image dimensions available, as well as the output colormap
     * if we asked for color quantization.
     * In this example, we need to make an output work buffer of the right size.
     */
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    int width = cinfo.output_width;
    int height = cinfo.output_height;

    printf("%d %d\n", width, height);

    NIV::Window window(width, height);

    while (XPending(window.dpy) > 0)
    {
        XEvent event;
        XNextEvent(window.dpy, &event);
        printf("Event %d\n", event.type);
        switch (event.type)
        {
            case Expose:
                printf("Expose\n");
                break;
            case ConfigureNotify:
                glViewport(0, 0, (GLint)event.xconfigure.width, event.xconfigure.height);
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glOrtho(0, event.xconfigure.width, event.xconfigure.height, 0, -1, 1);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                printf("configure\n");
        }
    }

    glClearColor(1.0, 0.5, 0.5, 1);
    glEnable(GL_TEXTURE_2D);

    GLuint image_tex;
    glGenTextures(1, &image_tex);
    glBindTexture(GL_TEXTURE_2D, image_tex);

    unsigned char rnd[1024];
    for(int i=0; i<1024; i++)
        rnd[i] = rand() % 256;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    for(int row=0; row<1024; row++)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, row, 1024, 1, GL_RGB, GL_UNSIGNED_BYTE, rnd);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    /*
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE))
        critical_error("Couldn't initialize SDL");

    SDL_Surface *screen = SDL_SetVideoMode(width, height, 0, SDL_SWSURFACE);

    if (!screen)
        critical_error("Couldn't create window");

    SDL_LockSurface(screen);
    for(int y=0; y<height; y++)
    {
        for(int x=0; x<width; x++)
        {
            put_pixel(screen, x, y, 128, 128, 128);
        }
    }
    SDL_UnlockSurface(screen);
    SDL_Flip(screen);

        */

    for(;;)
    //while(!jpeg_input_complete(&cinfo))
    {
        Ticks before_decode = Ticks::now();

        int consumed;
        do
            consumed = jpeg_consume_input(&cinfo);
        while(consumed != JPEG_REACHED_EOI && consumed != JPEG_SUSPENDED);

        //cinfo.dct_method = JDCT_IFAST;
        //cinfo.two_pass_quantize = FALSE;
        //cinfo.colormap = NULL;

        jpeg_start_output(&cinfo, cinfo.input_scan_number);
        printf("%d\n", cinfo.rec_outbuf_height);

        Ticks after_decode = Ticks::now();
        printf("decode time: %f\n", (double)(after_decode - before_decode) / (double)1000000);

        Ticks before = Ticks::now();

        while(cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_scanlines(&cinfo, buffer, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, cinfo.output_scanline-1, 1024, 1, GL_RGB, GL_UNSIGNED_BYTE, buffer[0]);
        }

        Ticks after = Ticks::now();
        //printf("display time: %f\n", (double)(after - before) / (double)1000000);

        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(0, 0);
        glTexCoord2f((double)width / (double)1024, 0);
        glVertex2f(width, 0);
        glTexCoord2f((double)width / (double)1024, (double)height / (double)1024);
        glVertex2f(width, height);
        glTexCoord2f(0, (double)height / (double)1024);
        glVertex2f(0, height);
        glEnd();
        glXSwapBuffers(window.dpy, window.win);

        /*
        SDL_LockSurface(screen);
        while(cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_scanlines(&cinfo, buffer, 1);
            for(int x=0; x<row_stride; x += 3)
                put_pixel(screen, x/3, cinfo.output_scanline - 1, buffer[0][x], buffer[0][x+1], buffer[0][x+2]);
        }
        SDL_UnlockSurface(screen);
        SDL_Flip(screen);
        */

        jpeg_finish_output(&cinfo);
        if (jpeg_input_complete(&cinfo) && cinfo.input_scan_number == cinfo.output_scan_number)
            break;
    }

//    /* Step 6: while (scan lines remain to be read) */
//    /*           jpeg_read_scanlines(...); */
//
//    /* Here we use the library's state variable cinfo.output_scanline as the
//     * loop counter, so that we don't have to keep track ourselves.
//     */
//    while (cinfo.output_scanline < cinfo.output_height) {
//        /* jpeg_read_scanlines expects an array of pointers to scanlines.
//         * Here the array is only one element long, but you could ask for
//         * more than one scanline at a time if that's more convenient.
//         */
//        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
//        /* Assume put_scanline_someplace wants a pointer and sample count. */
//        /*put_scanline_someplace(buffer[0], row_stride);*/
//
//        //printf("%d\n", cinfo.output_scanline);
//
//        SDL_LockSurface(screen);
//        for(int x=0; x<row_stride; x += 3)
//            put_pixel(screen, x/3, cinfo.output_scanline - 1, buffer[0][x], buffer[0][x+1], buffer[0][x+2]);
//        SDL_UnlockSurface(screen);
//
//    }
//    SDL_Flip(screen);

    /* Step 7: Finish decompression */

    while(jpeg_finish_decompress(&cinfo) == FALSE);

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

    /* After finish_decompress, we can close the input file.
     * Here we postpone it until after no more JPEG errors are possible,
     * so as to simplify the setjmp error logic above.  (Actually, I don't
     * think that jpeg_destroy can do an error exit, but why assume anything...)
     */
//    fclose(fp);

    /* At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */

    sleep(2);

    /*
    SDL_Quit();
    */

    return 0;
}

